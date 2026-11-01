# Cobrar y dar vuelto

La ventana **Cobrar** se abre con **F4** desde la venta. Arriba está el **total a cobrar** en la moneda base (normalmente CUP).

## Formas de pago

Se puede cobrar de tres maneras:

- **Efectivo**: escribe lo que entrega el cliente.
- **Transferencia**: escribe el importe transferido y, si quieres, el número de transacción (sale en el ticket y en los reportes).
- **Mixto**: una parte en transferencia y el resto en efectivo. Escribe los dos importes.

Debajo se calcula al momento:

- **Falta** (en rojo): lo pagado no alcanza todavía.
- **Vuelto** (en verde): lo que hay que devolver al cliente.

La transferencia no puede ser mayor que el total: el vuelto siempre sale del efectivo.

## Botones de importe rápido

Debajo del efectivo hay botones con importes típicos: **Exacto** (el total justo) y el total redondeado hacia arriba a 100, 500 y 1000. Un clic los pone en el campo de efectivo.

## Cobrar en otra moneda (USD, MLC…)

Si en [Monedas y tasas](ayuda:monedas) hay otras monedas activas, junto al efectivo aparece la lista de monedas:

1. Pulsa **F6** para pasar a la siguiente moneda (o elígela en la lista).
2. Escribe cuánto entrega el cliente **en esa moneda**. Debajo se muestra la tasa: `1 USD = 400.00 CUP`.
3. El programa convierte a la moneda base y calcula el vuelto.

El **vuelto siempre se da en la moneda base**. Cada pago guarda la tasa que se usó, así que cambiar la tasa después no altera las ventas anteriores.

Ejemplo: total 1 250 CUP, el cliente paga con 5 USD a 400 → recibes 2 000 CUP equivalentes → vuelto 750 CUP.

## Confirmar

- **Enter** (o *Confirmar venta*, o Ctrl+S) registra la venta. Solo se puede confirmar cuando no falta dinero.
- **Esc** (o *Volver*) regresa al carrito sin cobrar.
- La casilla **Agregar ticket al PDF del día** (o **Imprimir ticket**, si la salida es la impresora) decide si se genera el ticket de esta venta. Su valor inicial se configura en [Configuración](ayuda:configuracion) → *Al cobrar*.

La venta, sus líneas, los pagos y la rebaja del stock se guardan **juntos**: o se guarda todo o no se guarda nada. Un corte de luz en ese momento no deja ventas a medias.

## Stock insuficiente

Si algún producto quedaría con stock negativo, aparece una advertencia con la lista:

- **No vender (Esc)**: vuelve al cobro sin registrar nada.
- **Vender igual (Enter)**: registra la venta y el stock queda negativo. Corrígelo luego con una entrada o un ajuste en [Inventario](ayuda:inventario).

## Mensajes frecuentes

| Mensaje | Qué hacer |
|---|---|
| Falta por pagar … | Aumenta el efectivo o la transferencia |
| La transferencia no puede superar el total | Baja la transferencia; el vuelto sale del efectivo |
| No hay caja abierta | Abre la caja (F8) |
| Venta guardada, pero no se generó el ticket | La venta está bien; revisa la impresora o que el PDF del día no esté abierto. Reimprime con F10 |
