# Inventario: entradas y ajustes

La pantalla **Inventario** (Ctrl+3, solo administrador) controla cuánto hay de cada producto. El stock **baja solo al vender** y sube o se corrige con **entradas** y **ajustes**. Cada cambio queda en el historial con fecha, usuario y motivo.

## La pantalla

- **Izquierda**: los productos con su stock y su mínimo. Buscador (F2) por nombre o código; también se puede escanear.
- **Solo stock bajo**: deja solo los productos que están en su mínimo o por debajo. El número entre paréntesis (también en la barra izquierda) dice cuántos son.
- **Derecha**: el producto elegido con su stock actual en grande y su historial de **movimientos**.
- **Todos los productos**: el historial muestra los movimientos de todos, no solo del elegido.

## Entrada de mercancía (F5)

Cuando llega mercancía (compra, donación, producción propia):

1. Elige el producto y pulsa **F5**, **Enter**, doble clic o *Entrada*.
2. **Cantidad**: lo que entra (acepta decimales para kg, lb, l, m).
3. **Costo unitario**: opcional; viene con el costo actual.
4. **Actualizar el costo del producto**: si está marcado, el nuevo costo pasa a ser el costo del producto (afecta la ganancia de las ventas siguientes).
5. **Proveedor / nota**: opcional, por ejemplo el nombre del proveedor o el número de factura.
6. **Enter** registra la entrada. Antes se muestra cuánto quedará en stock.

## Ajuste de stock (F6)

Para corregir el stock cuando no coincide con lo que hay de verdad:

1. Elige el producto y pulsa **F6** o *Ajuste*.
2. **Stock contado**: la cantidad **real** que contaste (no la diferencia).
3. **Motivo**: conteo físico, merma, rotura o vencido, consumo interno, corrección u otro.
4. **Nota**: opcional.
5. Se muestra la diferencia (+ en verde, − en rojo). **Enter** aplica.

## Historial de movimientos

| Columna | Qué es |
|---|---|
| Fecha | Cuándo ocurrió |
| Tipo | Venta, Entrada o Ajuste |
| Cantidad | En verde si sumó, en rojo si restó |
| Saldo | El stock que quedó después |
| Detalle | Nota o motivo, costo (en entradas) y usuario |

## Stock negativo

Si se vende más de lo que el sistema tenía (con confirmación al cobrar), el stock queda negativo y se ve en rojo. Corrígelo con una **entrada** si llegó mercancía sin registrar, o con un **ajuste** si el conteo estaba mal.

## Consejos

- Al empezar a usar el programa, registra una entrada por producto con lo que tienes.
- Haz un conteo físico de vez en cuando y corrige con ajustes de motivo *Conteo físico*.
- Pon un **stock mínimo** a lo que no puede faltar: el aviso rojo te recuerda reponerlo.
