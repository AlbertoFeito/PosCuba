-- 006: clientes con venta a credito (fiado) y gastos del negocio

CREATE TABLE customers (
  id                 INTEGER PRIMARY KEY,
  name               TEXT NOT NULL UNIQUE COLLATE NOCASE,
  phone              TEXT,
  note               TEXT,
  credit_limit_cents INTEGER NOT NULL DEFAULT 0 CHECK (credit_limit_cents >= 0),  -- 0 = sin limite
  active             INTEGER NOT NULL DEFAULT 1,
  created_at         TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

-- Venta a credito: la parte que queda debiendo el cliente (no es un pago: no entra en payments).
ALTER TABLE sales ADD COLUMN customer_id INTEGER REFERENCES customers(id);
ALTER TABLE sales ADD COLUMN credit_cents INTEGER NOT NULL DEFAULT 0;
CREATE INDEX ix_sales_customer ON sales(customer_id);

-- Devolucion que rebaja la deuda del cliente en lugar de entregar dinero.
ALTER TABLE refunds ADD COLUMN to_account INTEGER NOT NULL DEFAULT 0;

-- Cuenta del cliente. Saldo = cargos - abonos - devoluciones.
CREATE TABLE customer_entries (
  id           INTEGER PRIMARY KEY,
  customer_id  INTEGER NOT NULL REFERENCES customers(id),
  kind         TEXT NOT NULL CHECK (kind IN ('charge','payment','refund')),
  amount_cents INTEGER NOT NULL CHECK (amount_cents > 0),
  sale_id      INTEGER REFERENCES sales(id),
  refund_id    INTEGER REFERENCES refunds(id),
  session_id   INTEGER REFERENCES cash_sessions(id),         -- abonos: caja donde entro el dinero
  method       TEXT CHECK (method IS NULL OR method IN ('cash','transfer')),
  note         TEXT,
  user_id      INTEGER NOT NULL REFERENCES users(id),
  created_at   TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
CREATE INDEX ix_centries_customer ON customer_entries(customer_id);
CREATE INDEX ix_centries_session ON customer_entries(session_id);

-- Gastos
CREATE TABLE expense_categories (
  id     INTEGER PRIMARY KEY,
  name   TEXT NOT NULL UNIQUE COLLATE NOCASE,
  active INTEGER NOT NULL DEFAULT 1
);
INSERT INTO expense_categories (name) VALUES
  ('Alquiler'), ('Electricidad'), ('Agua y gas'), ('Salarios'), ('Transporte'),
  ('Impuestos y tasas'), ('Mantenimiento'), ('Compras menores'), ('Otros');

-- Gastos fijos que se repiten cada mes
CREATE TABLE expense_recurring (
  id            INTEGER PRIMARY KEY,
  category_id   INTEGER NOT NULL REFERENCES expense_categories(id),
  description   TEXT NOT NULL,
  amount_cents  INTEGER NOT NULL CHECK (amount_cents > 0),
  day_of_month  INTEGER NOT NULL CHECK (day_of_month BETWEEN 1 AND 31),
  start_month   TEXT NOT NULL,          -- 'yyyy-MM': primer mes que se genera
  last_month    TEXT,                   -- ultimo mes generado
  active        INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE expenses (
  id               INTEGER PRIMARY KEY,
  expense_date     TEXT NOT NULL,       -- 'yyyy-MM-dd'
  category_id      INTEGER NOT NULL REFERENCES expense_categories(id),
  description      TEXT NOT NULL,
  amount_cents     INTEGER NOT NULL CHECK (amount_cents > 0),
  from_cash        INTEGER NOT NULL DEFAULT 0,
  cash_movement_id INTEGER REFERENCES cash_movements(id),
  recurring_id     INTEGER REFERENCES expense_recurring(id),
  user_id          INTEGER REFERENCES users(id),
  created_at       TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
CREATE INDEX ix_expenses_date ON expenses(expense_date);
