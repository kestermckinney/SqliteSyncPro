# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
# Configure (from repo root)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build everything (library + tests + example)
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test binary directly
./build/tests/tst_schemainspector
./build/tests/tst_authmanager
./build/tests/tst_syncengine
```

Qt Creator: open `CMakeLists.txt` at the repo root; Qt Creator will configure and list all targets automatically. Test targets appear under "Projects → Build Steps" after CMake configures.

## Architecture

The library is in `src/` and built as a static library (`SqliteSyncProLib`). All public API goes through `SqliteSyncPro` (the façade class). Internal components are not meant to be used directly by callers.

```
SqliteSyncPro          ← public façade (sqlitesyncpro.h/.cpp)
 ├── AuthManager       ← JWT authentication against PostgREST (authmanager.h/.cpp)
 ├── HttpClient        ← synchronous HTTP wrapper around QNetworkAccessManager (httpclient.h/.cpp)
 │    └── MockHttpClient (tests only)  ← overrides executeRequest() for unit tests
 └── SyncEngine        ← core push/pull logic (syncengine.h/.cpp)
      └── SchemaInspector  ← runtime SQLite schema inspection + JSON serialisation (schemainspector.h/.cpp)
```

### Postgres storage model

All SQLite tables are stored in a **single** Postgres table (`sync_data` by default):

| Column | Type | Purpose |
|---|---|---|
| `userid` | TEXT | Identifies the record owner; enforced by RLS |
| `tablename` | TEXT | Name of the originating SQLite table |
| `id` | TEXT | Value of the SQLite row's id column |
| `updateddate` | BIGINT | Last-modified UTC milliseconds |
| `jsonrowdata` | JSONB | All SQLite columns serialised as JSON (id, updateddate, and syncdate excluded) |

Primary key is `(userid, tablename, id)`.

This design means the Postgres schema **never changes** when new SQLite tables or columns are added — the JSONB field absorbs any structure.  PostgREST exposes the single `sync_data` endpoint; push uses `tablename` + `id` filters and pull uses `tablename` + `updateddate` filters (RLS restricts all queries to the authenticated user's rows automatically).

Call `api.setUserId("…")` after authenticating. Call `api.setPostgresTableName("…")` only if the table was created with a different name.

### Data structures

- `SyncTableConfig` (`syncconfig.h`) – per-table settings: table name, batch size, column name overrides.
- `SyncResult` / `TableSyncResult` (`syncresult.h`) – outcome of a sync call.

### Required SQLite columns

Every synced table must have three columns:

| Column | Type | Purpose |
|---|---|---|
| `id` | TEXT (UUID) | Unique record key across all devices |
| `updateddate` | INTEGER | Last-modified UTC timestamp in milliseconds |
| `syncdate` | INTEGER (nullable) | Local tracking: NULL = never synced; equals updateddate when clean |

`syncdate` is **local-only** and is never sent to Postgres.  The Postgres tables mirror the SQLite schema minus `syncdate`.

### Sync algorithm

**Push phase** (local → server, per batch):
1. `SELECT * FROM table WHERE syncdate IS NULL OR syncdate < updateddate LIMIT batchSize`
2. For each row, serialise to JSON (excluding `syncdate`).
3. `GET /table?id=eq.<id>` to check server state.
   - Row absent on server → `POST /table`
   - Row present, local `updateddate` newer → `PATCH /table?id=eq.<id>`
   - Row present, server newer → conflict; server wins; defer to pull phase.
4. On success, `UPDATE table SET syncdate = updateddate WHERE id = <id>`.

**Pull phase** (server → local, per batch):
1. Read `last_pull_time` from `_sync_meta` (created automatically in SQLite).
2. `GET /table?updateddate=gt.<last_pull_time>&order=updateddate.asc&limit=batchSize`
3. For each server record, upsert locally; winner determined by `updateddate`.
4. Set local `syncdate = server updateddate`.
5. Update `_sync_meta.last_pull_time` to the maximum `updateddate` seen.

### Multi-user isolation

Postgres RLS policies on `sync_data` filter by the `userid` column, which the client sends explicitly and the JWT `sub` claim independently validates. Each device authenticates with its own JWT; users never see each other's records. See `examples/invoice/postgres_schema.sql`.

### HttpClient testability

`HttpClient::executeRequest()` is `virtual` and `protected`. `MockHttpClient` (in `tests/`) overrides it to return pre-registered responses keyed by URL substring, with no real network calls.  Use `MockHttpClient::recordedRequests` to assert what the engine actually sent.

### Shared database lock

The calling application **must** use the same `QReadWriteLock` as the sync engine to coordinate its own database access:

```cpp
// Get the lock (or supply your own with setDatabaseLock())
QReadWriteLock *lock = api.databaseLock();

// App reading from SQLite:
{ QReadLocker rl(lock);  db.exec("SELECT ..."); }

// App writing to SQLite:
{ QWriteLocker wl(lock); db.exec("INSERT ..."); }
```

The sync engine acquires a **write lock** for each table batch and releases it between tables (never held during network I/O). This means:
- App reads are blocked only while a table batch is actively writing to SQLite
- App writes are blocked only while a table batch is running (push + pull for one table)
- Multiple app read threads can proceed concurrently when no sync is writing

If the application already has a `QReadWriteLock`, pass it in before starting any sync: `api.setDatabaseLock(&myLock)`. Passing `nullptr` resets to the internal lock.

### Threading model

`SqliteSyncPro` is thread-safe at the configuration level (`QMutex` protects all member variables).  Sync operations are thread-safe because each call creates its own resources:

| Resource | Per-call? | Why |
|---|---|---|
| `QSqlDatabase` | Yes – unique UUID connection name | Qt requires each thread to have its own connection |
| `HttpClient` / `QNetworkAccessManager` | Yes – created in the calling thread | `QNAM` has thread affinity |
| `SyncEngine` | Yes – stack-allocated per call | No shared state |

**WAL mode** is enabled on `openDatabase()` and persists in the database file. This allows multiple concurrent readers alongside one writer so the UI thread can read while a background sync writes.

**Application code** that accesses the SQLite file from multiple threads must also open per-thread `QSqlDatabase` connections with unique connection names. Sharing a single `QSqlDatabase` across threads is not supported by Qt.

`synchronize()` blocks the calling thread — call it from a dedicated worker thread, not the UI thread.  `synchronizeAsync()` spawns a `QThread` internally and returns immediately; connect to `syncProgress` / `syncCompleted` signals (queued connection) to receive results.

`SyncWorker` (`syncworker.h/.cpp`) is the internal class used by `synchronizeAsync()`. It can also be used directly if finer control over the worker thread lifecycle is needed.

## SQLSync Administrator (`admin/`)

A standalone Qt6/QML desktop app that guides a non-technical user through setting up the Postgres database and managing sync users. It does **not** link against `SqliteSyncProLib`; it talks directly to Postgres as superuser via Qt's QPSQL driver.

```
admin/
 ├── main.cpp                        ← QML engine + AdminController context property
 ├── CMakeLists.txt                  ← Separate project; Qt6-only; uses qt_add_qml_module
 ├── backend/
 │    ├── AdminController.{h,cpp}   ← QML-facing singleton (Q_PROPERTY + Q_INVOKABLE)
 │    └── SetupWorker.{h,cpp}       ← 24-step DDL sequence on a dedicated QThread
 └── qml/
      ├── main.qml                  ← ApplicationWindow, sidebar + StackView
      ├── WizardSidebar.qml         ← Step indicators with active/done/disabled states
      └── pages/
           ├── ConnectionPage.qml   ← Credentials form + connection test
           ├── ConfigurePage.qml    ← Review what will be installed; triggers startSetup()
           ├── InstallPage.qml      ← Live log of 24 SQL steps with progress bar
           ├── SummaryPage.qml      ← Shows generated credentials + postgrest.conf snippet
           └── UsersPage.qml        ← Add/remove sync users via create_user() RPC
```

**Setup sequence** (`SetupWorker::runSetup`): step 0 (pgcrypto) runs outside any transaction because `CREATE EXTENSION` cannot run inside a transaction on older Postgres versions. Steps 1–23 run inside an explicit `BEGIN`/`COMMIT`; any failure triggers a `goto rollback` → `ROLLBACK`. All role/policy DO blocks are idempotent (catch `duplicate_object`).

**Secrets**: `generateSecrets()` produces URL-safe base64 strings using `QRandomGenerator::securelySeeded()`. The alphabet `[A-Za-z0-9-_]` contains no SQL metacharacters, making direct string substitution safe in the DDL.

**Navigation**: `root.currentStep` (int) drives both the sidebar highlight and `StackView` content. Pages call `root.nextPage()` or `root.goToPage(n)`. `main.qml` auto-advances to SummaryPage on `Admin.setupFinished(true, …)`.

## Key design decisions

- **Synchronous HTTP** – `HttpClient` uses `QEventLoop` inside each call so the API presents a simple blocking interface. This is intentional; sync is a sequential batch operation.
- **UUID IDs** – UUIDs generated on the device avoid primary key conflicts when multiple devices create records offline.
- **Batch size** – `synchronize()` is called repeatedly by the app until it returns 0 pushed and 0 pulled to drain the backlog.
- **No syncdate on server** – Postgres tables do not have `syncdate`. It is a local-only dirty-tracking mechanism.
- **Dynamic schema** – `SchemaInspector` uses `PRAGMA table_info` at runtime; no code generation or model registration is needed for new tables.
