-- 001: esquema inicial
-- Dinero: INTEGER en centavos de la moneda base. Cantidades: INTEGER en milesimas.
-- Tasas: INTEGER en milesimas (1000 = 1.0).

CREATE TABLE settings (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE currencies (
  code       TEXT PRIMARY KEY,
  name       TEXT NOT NULL,
  symbol     TEXT NOT NULL,
  rate_milli INTEGER NOT NULL DEFAULT 1000 CHECK (rate_milli > 0),
  is_base    INTEGER NOT NULL DEFAULT 0,
  active     INTEGER NOT NULL DEFAULT 1
);
CREATE UNIQUE INDEX ux_currency_base ON currencies(is_base) WHERE is_base = 1;

CREATE TABLE users (
  id         INTEGER PRIMARY KEY,
  name       TEXT NOT NULL UNIQUE,
  pin_hash   TEXT NOT NULL,
  pin_salt   TEXT NOT NULL,
  role       TEXT NOT NULL CHECK (role IN ('admin','cashier')),
  active     INTEGER NOT NULL DEFAULT 1,
  created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE categories (
  id     INTEGER PRIMARY KEY,
  name   TEXT NOT NULL UNIQUE,
  active INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE products (
  id              INTEGER PRIMARY KEY,
  code            TEXT NOT NULL UNIQUE,
  barcode         TEXT UNIQUE,
  name            TEXT NOT NULL,
  category_id     INTEGER REFERENCES categories(id),
  cost_cents      INTEGER NOT NULL DEFAULT 0 CHECK (cost_cents >= 0),
  price_cents     INTEGER NOT NULL DEFAULT 0 CHECK (price_cents >= 0),
  unit            TEXT NOT NULL DEFAULT 'u',
  stock_milli     INTEGER NOT NULL DEFAULT 0,
  min_stock_milli INTEGER NOT NULL DEFAULT 0,
  active          INTEGER NOT NULL DEFAULT 1,
  updated_at      TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
CREATE INDEX ix_products_name ON products(name COLLATE NOCASE);
CREATE INDEX ix_products_category ON products(category_id);

CREATE TABLE cash_sessions (
  id               INTEGER PRIMARY KEY,
  opened_by        INTEGER NOT NULL REFERENCES users(id),
  closed_by        INTEGER REFERENCES users(id),
  opened_at        TEXT NOT NULL DEFAULT (datetime('now','localtime')),
  closed_at        TEXT,
  opening_cents    INTEGER NOT NULL CHECK (opening_cents >= 0),
  expected_cents   INTEGER,
  counted_cents    INTEGER,
  difference_cents INTEGER,
  status           TEXT NOT NULL DEFAULT 'open' CHECK (status IN ('open','closed'))
);
CREATE UNIQUE INDEX ux_one_open_session ON cash_sessions(status) WHERE status = 'open';

CREATE TABLE cash_movements (
  id           INTEGER PRIMARY KEY,
  session_id   INTEGER NOT NULL REFERENCES cash_sessions(id),
  user_id      INTEGER NOT NULL REFERENCES users(id),
  amount_cents INTEGER NOT NULL,
  reason       TEXT NOT NULL,
  created_at   TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
CREATE INDEX ix_cashmov_session ON cash_movements(session_id);

CREATE TABLE sales (
  id             INTEGER PRIMARY KEY,
  number         INTEGER NOT NULL UNIQUE,
  session_id     INTEGER NOT NULL REFERENCES cash_sessions(id),
  user_id        INTEGER NOT NULL REFERENCES users(id),
  subtotal_cents INTEGER NOT NULL,
  discount_cents INTEGER NOT NULL DEFAULT 0,
  total_cents    INTEGER NOT NULL,
  change_cents   INTEGER NOT NULL DEFAULT 0,
  status         TEXT NOT NULL DEFAULT 'completed' CHECK (status IN ('completed','voided')),
  created_at     TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
CREATE INDEX ix_sales_date ON sales(created_at);
CREATE INDEX ix_sales_session ON sales(session_id);

CREATE TABLE sale_items (
  id               INTEGER PRIMARY KEY,
  sale_id          INTEGER NOT NULL REFERENCES sales(id) ON DELETE CASCADE,
  product_id       INTEGER NOT NULL REFERENCES products(id),
  product_name     TEXT NOT NULL,
  qty_milli        INTEGER NOT NULL CHECK (qty_milli > 0),
  price_cents      INTEGER NOT NULL,
  cost_cents       INTEGER NOT NULL,
  discount_cents   INTEGER NOT NULL DEFAULT 0,
  line_total_cents INTEGER NOT NULL
);
CREATE INDEX ix_items_sale ON sale_items(sale_id);
CREATE INDEX ix_items_product ON sale_items(product_id);

CREATE TABLE payments (
  id            INTEGER PRIMARY KEY,
  sale_id       INTEGER NOT NULL REFERENCES sales(id) ON DELETE CASCADE,
  method        TEXT NOT NULL CHECK (method IN ('cash','transfer')),
  currency_code TEXT NOT NULL REFERENCES currencies(code),
  amount_cents  INTEGER NOT NULL,
  rate_milli    INTEGER NOT NULL,
  base_cents    INTEGER NOT NULL,
  reference     TEXT
);
CREATE INDEX ix_payments_sale ON payments(sale_id);

CREATE TABLE stock_movements (
  id         INTEGER PRIMARY KEY,
  product_id INTEGER NOT NULL REFERENCES products(id),
  type       TEXT NOT NULL CHECK (type IN ('purchase','sale','adjust','void')),
  qty_milli  INTEGER NOT NULL,
  cost_cents INTEGER,
  sale_id    INTEGER REFERENCES sales(id),
  user_id    INTEGER NOT NULL REFERENCES users(id),
  note       TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
CREATE INDEX ix_mov_product_date ON stock_movements(product_id, created_at);

-- Datos iniciales
INSERT INTO currencies (code, name, symbol, rate_milli, is_base) VALUES ('CUP', 'Peso cubano', '$', 1000, 1);
INSERT INTO settings (key, value) VALUES ('base_currency', 'CUP');
INSERT INTO settings (key, value) VALUES ('business_name', 'Mi negocio');
INSERT INTO settings (key, value) VALUES ('theme', 'light');
