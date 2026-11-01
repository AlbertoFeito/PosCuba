# Caja: apertura, movimientos y cierre

La pantalla **Caja** (Ctrl+4) controla el efectivo de la gaveta. Solo puede haber **una caja abierta** a la vez. Los cajeros también tienen acceso a esta pantalla.

## Abrir la caja (F8)

1. Pulsa **Abrir caja (F8)**, aquí o en Venta.
2. Escribe el **fondo inicial**: el efectivo que hay en la gaveta para dar vuelto (puede ser 0).
3. **Enter**.

Sin caja abierta no se puede vender.

## Mientras está abierta

La pantalla muestra quién la abrió, cuándo y con qué fondo, y estos totales:

| Recuadro | Qué es |
|---|---|
| Ventas | Cantidad de ventas desde la apertura |
| Total vendido | Suma de las ventas en la moneda base |
| Ganancia estimada | Total vendido menos el costo de lo vendido (usa el costo de cada producto) |
| Transferencias | Lo cobrado por transferencia (no está en la gaveta) |
| Efectivo por ventas | Lo cobrado en efectivo (todas las monedas convertidas a la base), ya descontado el vuelto |
| Ingresos manuales | Dinero que se agregó a la gaveta sin ser venta |
| Retiros | Dinero que se sacó de la gaveta |
| Devoluciones | Dinero devuelto a clientes en esta caja (ver [Devoluciones](ayuda:devoluciones)). Ya está restado del total vendido, de la ganancia y del efectivo esperado |

Debajo, **Efectivo esperado en la gaveta**, uno por moneda:

- Moneda base: fondo inicial + efectivo de ventas − vuelto + ingresos − retiros − devoluciones en efectivo.
- Otras monedas (USD, MLC…): lo recibido en esa moneda.

## Ingresos y retiros

Para mover efectivo que no es una venta:

- **Retiro**: pago a un proveedor, depósito en el banco, dinero que se lleva el dueño.
- **Ingreso**: cambio adicional que se agrega a la gaveta.

Escribe el **importe** y el **motivo** (obligatorio). Son siempre en la moneda base. No se puede retirar más efectivo del que hay.

## Cerrar la caja: arqueo (F9)

1. Pulsa **Cerrar caja (F9)**.
2. Cuenta el dinero de la gaveta y escribe el **total contado de cada moneda**. Enter pasa a la siguiente.
3. Al lado de cada una se ve la **diferencia** con lo esperado:
   - en verde, 0: la caja cuadra;
   - en rojo, negativa: **falta** dinero;
   - en amarillo, positiva: **sobra** dinero.
4. **Cerrar caja** registra el cierre.

El resumen final muestra el esperado, el contado y la diferencia en moneda base (las otras monedas se convierten con su tasa).

Al cerrar la caja se hace un **respaldo automático** de los datos (si está activado). Ver [Respaldos](ayuda:respaldos).

## Cierres anteriores

La tabla de abajo muestra cada cierre: apertura, cierre, usuario, esperado, contado y diferencia, con el mismo código de colores.

## Consejos

- Abre la caja con el fondo real contado, no "de memoria".
- Registra cada salida de dinero como retiro en el momento; si no, el cierre dará faltante.
- Si una diferencia se repite, revisa los vueltos dados en otra moneda y las transferencias cobradas como efectivo.
