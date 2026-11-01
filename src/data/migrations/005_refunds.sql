-- 005: devoluciones y anulaciones
-- Una devolucion reintegra parte (o todo) de una venta. El dinero sale de la caja abierta
-- en ese momento (efectivo en moneda base) o se devuelve por transferencia.
-- Si se devuelve todo lo vendido, la venta queda con status = 'voided' (anulada).
CREATE TABLE refunds (
  id            INTEGER PRIMARY KEY,
  number        INTEGER NOT NULL UNIQUE,
  sale_id       INTEGER NOT NULL REFERENCES sales(id),
  session_id    INTEGER NOT NULL REFERENCES cash_sessions(id),
  user_id       INTEGER NOT NULL REFERENCES users(id),
  authorized_by INTEGER NOT NULL REFERENCES users(id),
  method        TEXT NOT NULL CHECK (method IN ('cash','transfer')),
  reason        TEXT NOT NULL,
  restock       INTEGER NOT NULL DEFAULT 1,
  total_cents   INTEGER NOT NULL CHECK (total_cents >= 0),
  cost_cents    INTEGER NOT NULL DEFAULT 0,
  created_at    TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);
CREATE INDEX ix_refunds_sale ON refunds(sale_id);
CREATE INDEX ix_refunds_session ON refunds(session_id);
CREATE INDEX ix_refunds_date ON refunds(created_at);

CREATE TABLE refund_items (
  id            INTEGER PRIMARY KEY,
  refund_id     INTEGER NOT NULL REFERENCES refunds(id) ON DELETE CASCADE,
  sale_item_id  INTEGER NOT NULL REFERENCES sale_items(id),
  product_id    INTEGER NOT NULL REFERENCES products(id),
  qty_milli     INTEGER NOT NULL CHECK (qty_milli > 0),
  amount_cents  INTEGER NOT NULL,
  cost_cents    INTEGER NOT NULL
);
CREATE INDEX ix_refund_items_refund ON refund_items(refund_id);
CREATE INDEX ix_refund_items_sale_item ON refund_items(sale_item_id);
