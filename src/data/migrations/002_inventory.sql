-- 002: inventario
-- Saldo despues de cada movimiento (para el historial sin recalcular).
ALTER TABLE stock_movements ADD COLUMN stock_after_milli INTEGER;
CREATE INDEX ix_mov_date ON stock_movements(created_at);

-- Usuario administrador inicial. pin_hash vacio = debe definir PIN en el primer inicio (hito de usuarios).
INSERT INTO users (name, pin_hash, pin_salt, role)
SELECT 'Administrador', '', '', 'admin' WHERE NOT EXISTS (SELECT 1 FROM users);
