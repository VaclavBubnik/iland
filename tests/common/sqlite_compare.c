/* Compare two iLand output databases without touching either model input. */
#include "sqlite3.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail_db(sqlite3 *db, const char *action)
{
    fprintf(stderr, "ERROR: %s: %s\n", action, db ? sqlite3_errmsg(db) : "SQLite error");
    exit(2);
}

static sqlite3 *open_database(const char *path)
{
    sqlite3 *db = NULL;
    if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK)
        fail_db(db, path);
    return db;
}

static sqlite3_stmt *prepare(sqlite3 *db, const char *sql)
{
    sqlite3_stmt *statement = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &statement, NULL) != SQLITE_OK)
        fail_db(db, sql);
    return statement;
}

static int is_number(int type)
{
    return type == SQLITE_INTEGER || type == SQLITE_FLOAT;
}

static int values_equal(sqlite3_stmt *left, sqlite3_stmt *right, int column)
{
    const int left_type = sqlite3_column_type(left, column);
    const int right_type = sqlite3_column_type(right, column);

    if (is_number(left_type) && is_number(right_type)) {
        const double a = sqlite3_column_double(left, column);
        const double b = sqlite3_column_double(right, column);
        const double tolerance = 1e-9 + 1e-9 * fmax(fabs(a), fabs(b));
        return a == b || (isfinite(a) && isfinite(b) && fabs(a - b) <= tolerance);
    }
    if (left_type != right_type)
        return 0;
    if (left_type == SQLITE_NULL)
        return 1;
    if (left_type == SQLITE_BLOB) {
        const int left_size = sqlite3_column_bytes(left, column);
        const int right_size = sqlite3_column_bytes(right, column);
        return left_size == right_size &&
            memcmp(sqlite3_column_blob(left, column), sqlite3_column_blob(right, column),
                   (size_t)left_size) == 0;
    }
    return strcmp((const char *)sqlite3_column_text(left, column),
                  (const char *)sqlite3_column_text(right, column)) == 0;
}

static sqlite3_stmt *table_list(sqlite3 *db)
{
    return prepare(db,
        "SELECT name FROM sqlite_master "
        "WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name");
}

static int table_exists(sqlite3 *db, const char *name)
{
    sqlite3_stmt *statement = prepare(db,
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?1");
    sqlite3_bind_text(statement, 1, name, -1, SQLITE_TRANSIENT);
    const int exists = sqlite3_step(statement) == SQLITE_ROW;
    sqlite3_finalize(statement);
    return exists;
}

static sqlite3_stmt *table_columns(sqlite3 *db, const char *table)
{
    char *sql = sqlite3_mprintf("PRAGMA table_info(\"%w\")", table);
    sqlite3_stmt *statement = prepare(db, sql);
    sqlite3_free(sql);
    return statement;
}

static sqlite3_stmt *sorted_rows(sqlite3 *db, const char *table,
                                 sqlite3_stmt *columns, int column_count,
                                 int sort_rows)
{
    sqlite3_str *sql = sqlite3_str_new(db);
    sqlite3_stmt *statement;
    char *query;

    sqlite3_str_appendall(sql, "SELECT ");
    for (int i = 0; i < column_count; ++i) {
        const char *name;
        sqlite3_reset(columns);
        for (int row = 0; row <= i; ++row)
            sqlite3_step(columns);
        name = (const char *)sqlite3_column_text(columns, 1);
        sqlite3_str_appendf(sql, "%s\"%w\"", i ? "," : "", name);
    }
    sqlite3_str_appendf(sql, " FROM \"%w\"", table);
    if (sort_rows) {
        sqlite3_str_appendall(sql, " ORDER BY ");
        for (int i = 1; i <= column_count; ++i)
            sqlite3_str_appendf(sql, "%s%d", i == 1 ? "" : ",", i);
    }
    query = sqlite3_str_finish(sql);
    statement = prepare(db, query);
    sqlite3_free(query);
    return statement;
}

static int compare_table(sqlite3 *expected, sqlite3 *actual, const char *table,
                         int sort_rows)
{
    sqlite3_stmt *expected_columns = table_columns(expected, table);
    sqlite3_stmt *actual_columns = table_columns(actual, table);
    int column_count = 0;
    int schema_ok = 1;
    int mismatches = 0;
    long rows = 0;

    while (sqlite3_step(expected_columns) == SQLITE_ROW) {
        const char *expected_name = (const char *)sqlite3_column_text(expected_columns, 1);
        const int actual_step = sqlite3_step(actual_columns);
        ++column_count;
        if (actual_step != SQLITE_ROW ||
            strcmp(expected_name, (const char *)sqlite3_column_text(actual_columns, 1)) != 0) {
            fprintf(stderr, "%s: schema differs at column %d (%s)\n", table,
                    column_count, expected_name);
            schema_ok = 0;
            break;
        }
    }
    if (schema_ok && sqlite3_step(actual_columns) == SQLITE_ROW) {
        fprintf(stderr, "%s: workspace has additional columns\n", table);
        schema_ok = 0;
    }
    sqlite3_finalize(expected_columns);
    sqlite3_finalize(actual_columns);
    if (!schema_ok)
        return 1;

    expected_columns = table_columns(expected, table);
    actual_columns = table_columns(actual, table);
    sqlite3_stmt *expected_rows = sorted_rows(expected, table, expected_columns, column_count, sort_rows);
    sqlite3_stmt *actual_rows = sorted_rows(actual, table, actual_columns, column_count, sort_rows);
    sqlite3_finalize(expected_columns);
    sqlite3_finalize(actual_columns);

    int expected_done = 0;
    int actual_done = 0;
    for (;;) {
        const int left_step = expected_done ? SQLITE_DONE : sqlite3_step(expected_rows);
        const int right_step = actual_done ? SQLITE_DONE : sqlite3_step(actual_rows);
        if ((left_step != SQLITE_ROW && left_step != SQLITE_DONE) ||
            (right_step != SQLITE_ROW && right_step != SQLITE_DONE)) {
            fail_db(expected, "reading expected table rows");
        }
        expected_done = expected_done || left_step == SQLITE_DONE;
        actual_done = actual_done || right_step == SQLITE_DONE;
        if (left_step == SQLITE_DONE && right_step == SQLITE_DONE)
            break;
        ++rows;
        if (left_step != right_step) {
            if (mismatches < 8)
                fprintf(stderr, "%s: row count differs near sorted row %ld\n", table, rows);
            ++mismatches;
            continue;
        }
        for (int column = 0; column < column_count; ++column) {
            const char *column_name = sqlite3_column_name(expected_rows, column);
            if (strcmp(table, "runinfo") == 0 &&
                (strcmp(column_name, "timestamp") == 0 ||
                 strcmp(column_name, "version") == 0))
                continue;
            if (!values_equal(expected_rows, actual_rows, column)) {
                if (mismatches < 8)
                    fprintf(stderr, "%s row %ld column %s differs\n", table, rows,
                        column_name);
                ++mismatches;
            }
        }
    }
    sqlite3_finalize(expected_rows);
    sqlite3_finalize(actual_rows);
    printf("%s: %ld rows, %d differing values/rows\n", table, rows, mismatches);
    return mismatches != 0;
}

int main(int argc, char **argv)
{
    sqlite3 *expected;
    sqlite3 *actual;
    sqlite3_stmt *tables;
    int failed = 0;
    int table_count = 0;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <official-output.sqlite> <workspace-output.sqlite>\n", argv[0]);
        return 2;
    }
    expected = open_database(argv[1]);
    actual = open_database(argv[2]);
    tables = table_list(expected);
    while (sqlite3_step(tables) == SQLITE_ROW) {
        const char *table = (const char *)sqlite3_column_text(tables, 0);
        ++table_count;
        if (!table_exists(actual, table)) {
            fprintf(stderr, "%s: missing from workspace output\n", table);
            failed = 1;
            continue;
        }
        failed |= compare_table(expected, actual, table, 1);
        fflush(stdout);
        fflush(stderr);
    }
    sqlite3_finalize(tables);

    tables = table_list(actual);
    while (sqlite3_step(tables) == SQLITE_ROW) {
        const char *table = (const char *)sqlite3_column_text(tables, 0);
        if (!table_exists(expected, table)) {
            fprintf(stderr, "%s: extra table in workspace output\n", table);
            failed = 1;
        }
    }
    sqlite3_finalize(tables);
    if (table_count == 0) {
        fprintf(stderr, "No output tables found\n");
        failed = 1;
    }
    sqlite3_close(expected);
    sqlite3_close(actual);
    puts(failed ? "FAIL: iLand output databases differ" : "PASS: iLand output databases match");
    return failed ? 1 : 0;
}
