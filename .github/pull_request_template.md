# Description
Please include a summary of the change and which issue is fixed.


## Database/Flyway Migrations 
**How to use:**
Identify which **phase** of the Expand-Contract pattern your current ticket/PR covers. You only need to complete the section relevant to your specific change.

### 1. PHASE 1: EXPAND (Adding new things)
- [ ] My migration ONLY runs `CREATE TABLE`, `ADD COLUMN`, or `CREATE INDEX`.
- [ ] **Check**: I am NOT adding `NOT NULL` constraints to existing columns.
- [ ] **Status**: **SAFE** to deploy.

### 2. PHASE 2: MIGRATE (Data Sync / Code Switch)
- [ ] I am deploying code that writes to BOTH old and new columns/tables.
- [ ] I am moving the "Source of Truth" for reads to the new column.
- [ ] **Status**: **SAFE** to deploy (if code handles both).

### 3. PHASE 3: CONTRACT (Cleanup / Dropping old things)
- [ ] I am running a migration to DROP or RENAME a column/table.
- [ ] **Prerequisite**: Was Phase 1 (Expand) deployed to Prod in a **previous** release?
- [ ] **Prerequisite**: Was Phase 2 (Migrate) completed and is the data fully synced in Prod?
- [ ] **Prerequisite**: Has the code using the old column/table been completely removed in a **previous** release?
- [ ] **Status**: Only proceed if ALL prerequisites are met.

### 4. SPECIAL CASE: CONSTRAINTS (Adding NOT NULL or UNIQUE)
- [ ] See `docs/patterns/expand-contract-pattern.md` for special instructions.
