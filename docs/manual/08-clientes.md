# Clientes y fiado

La pantalla **Clientes** (Ctrl+Shift+C) lleva la cuenta de los clientes a los que se les vende **a crédito** (fiado): quién debe, cuánto y desde cuándo. Los cajeros también la usan: pueden crear clientes, ver cuentas y cobrar abonos.

## La pantalla

- Arriba: **Por cobrar**, la suma de todas las deudas, y cuántos clientes deben.
- A la izquierda, la lista: nombre, teléfono, lo que **debe** y su **límite**. Un cliente que pasó su límite se ve en rojo.
- **Buscar (F2)**: por nombre o teléfono. **Solo con deuda** deja solo a los que deben. **Inactivos** muestra también los desactivados.
- A la derecha, el cliente elegido: lo que debe y su **cuenta**, del movimiento más nuevo al más viejo, con el saldo después de cada uno.

En el menú lateral, **Clientes (N)** indica cuántos clientes tienen deuda.

## Crear o editar un cliente

1. **Nuevo cliente (Ctrl+N)**, o **Editar (F3)** / **Enter** sobre el elegido.
2. **Nombre** (obligatorio, no se puede repetir), **teléfono** y una **nota** (dirección, de parte de quién viene…).
3. **Límite de crédito** (solo el administrador): lo máximo que puede deber. Vacío o 0 = sin límite.
4. **Ctrl+S** o *Guardar*.

También se puede crear un cliente desde el cobro, con el botón **+** junto a la lista de clientes.

## Vender a crédito

Se hace en la ventana de cobro con **F9**. Ver [Cobrar](ayuda:cobro). El cliente puede pagar una parte en ese momento y fiar el resto.

Si el cliente tiene límite y la venta lo haría pasar, el programa no la deja confirmar. El administrador puede subir el límite en la ficha del cliente.

## Registrar un abono (F5)

Cuando el cliente viene a pagar todo o parte de lo que debe:

1. Elige el cliente y pulsa **Abono (F5)**.
2. Escribe el importe (no puede ser mayor que lo que debe).
3. Elige **Efectivo** o **Transferencia** y, si quieres, una nota.
4. **Enter**.

Hace falta tener la **caja abierta**. El abono en efectivo entra a la gaveta y se suma al efectivo esperado; el de transferencia no. Los dos aparecen en el recuadro **Abonos de clientes** de [Caja](ayuda:caja).

## La cuenta del cliente

| Movimiento | Qué es |
|---|---|
| Compra a crédito | Lo fiado en una venta (con su número). Sube la deuda |
| Abono | Un pago del cliente, en efectivo o transferencia. Baja la deuda |
| Devolución | Mercancía devuelta de una venta a crédito, devuelta **a la cuenta**. Baja la deuda |

Los movimientos no se borran ni se editan: si hubo un error, se corrige con un movimiento contrario (por ejemplo, devolver la venta).

## Devoluciones de ventas a crédito

En [Devoluciones](ayuda:devoluciones), si la venta fue fiada, aparece la opción **A la cuenta de …** y viene marcada: lo devuelto se resta de la deuda sin sacar dinero de la caja.

## Desactivar un cliente

**Desactivar** lo quita de la lista y del cobro, sin borrar su historial. Solo se puede si **no debe nada**. Con **Inactivos** marcado se ve y se puede reactivar.

## Exportar deudas

**Exportar deudas** (administrador) guarda un CSV para Excel con cada cliente que debe, su teléfono, lo que debe, su límite y la fecha del último movimiento. Se guarda en `Documentos\PosCuba\Reportes`.

## En los reportes

- Lo fiado cuenta como venta y como ganancia **el día que se vende**.
- [Reportes](ayuda:reportes) muestra el recuadro **A crédito** con lo vendido fiado y, debajo, los abonos cobrados en el período.
- El detalle de ventas en CSV incluye las columnas *A crédito* y *Cliente*.

## Mensajes frecuentes

| Mensaje | Qué hacer |
|---|---|
| Ya existe un cliente llamado … | Usa otro nombre (agrega el apellido o el barrio) |
| Supera el límite de crédito … | Cobra una parte más ahora o sube el límite |
| El abono no puede ser mayor que la deuda | Escribe como máximo lo que debe |
| No se puede desactivar: todavía debe … | Registra el abono o la devolución primero |
| Abre la caja para registrar abonos | Abre la caja (F8) |
