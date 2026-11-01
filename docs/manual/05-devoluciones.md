# Devoluciones y anulaciones

Cuando un cliente devuelve productos, o una venta se registró mal, no se borra la venta: se hace una **devolución**. Queda registrado qué se devolvió, cuánto dinero salió, quién lo hizo y quién lo autorizó.

- **Devolución parcial**: el cliente devuelve parte de lo que compró (una de tres colas, media libra de arroz).
- **Anulación**: se devuelve todo lo que queda de la venta. La venta queda marcada como **ANULADA**.

## Cómo se hace

1. En **Venta**, pulsa **F10** (Ventas recientes) y elige la venta.
2. Pulsa **Devolver / anular…** (o **Supr**).
3. Escribe en **A devolver** la cantidad de cada producto que se devuelve. Vacío o 0 = ese producto no se devuelve. Para devolverlo todo, marca **Devolver todo y anular la venta**.
4. Elige cómo se devuelve el dinero: **En efectivo** o **Por transferencia**.
5. Escribe el **motivo** (obligatorio): producto defectuoso, error al cobrar, el cliente se arrepintió…
6. Deja marcado **Reintegrar los productos al inventario** si se pueden volver a vender. Desmárcalo si están rotos o vencidos.
7. Si eres **cajero**, elige un administrador en **Autoriza** y que escriba su PIN.
8. **Confirmar devolución** (Ctrl+S). El mensaje dice cuánto dinero hay que entregar al cliente.

La caja debe estar abierta. Una devolución no se puede deshacer.

## Cuánto se devuelve

Abajo se ve **A devolver** antes de confirmar. Se calcula así:

- Por cada producto, la parte proporcional de su importe: 1 de 3 colas de 120.00 = 120.00.
- Si la venta tuvo **descuento**, se descuenta también la parte que le toca. Ejemplo: venta de 860.00 con 86.00 de descuento (10 %) → devolver una cola de 120.00 son **108.00**.
- Si con esta devolución ya no queda nada de la venta, se devuelve **exactamente lo que falta** del total cobrado, sin perder centavos por redondeo.
- Nunca se devuelve más de lo que se cobró.

## Qué cambia en el programa

| Dónde | Qué pasa |
|---|---|
| Caja | En efectivo: el importe sale del efectivo esperado de la caja abierta. Aparece en el recuadro **Devoluciones** y se resta del total vendido y de la ganancia |
| Inventario | Si se reintegra, el stock sube y queda un movimiento **Devolución** con el número de venta |
| Reportes | Lo devuelto se resta en la fecha de la devolución. Una venta anulada no cuenta como venta |
| Ticket | El ticket de la venta muestra cada devolución y el **NETO**. Con salida PDF, el PDF del día de esa venta se actualiza solo |
| Ventas recientes | La venta muestra *devuelto …* en amarillo, o *ANULADA* en rojo |

Una devolución en **efectivo** necesita que haya ese dinero en la gaveta. Si no alcanza, devuelve por transferencia o registra antes un **ingreso** en [Caja](ayuda:caja).

## Quién puede

- **Administrador**: hace la devolución directamente.
- **Cajero**: puede prepararla, pero un administrador debe autorizarla con su PIN. Los errores de PIN cuentan para el bloqueo de 30 segundos, igual que al entrar.

Ver también: [Vender](ayuda:venta), [Tickets](ayuda:tickets) y [Reportes](ayuda:reportes).
