-- 003: arqueo de caja por moneda
CREATE TABLE cash_counts (
  id             INTEGER PRIMARY KEY,
  session_id     INTEGER NOT NULL REFERENCES cash_sessions(id),
  currency_code  TEXT NOT NULL REFERENCES currencies(code),
  rate_milli     INTEGER NOT NULL,
  expected_cents INTEGER NOT NULL,
  counted_cents  INTEGER NOT NULL,
  UNIQUE (session_id, currency_code)
);
CREATE INDEX ix_payments_currency ON payments(currency_code);
