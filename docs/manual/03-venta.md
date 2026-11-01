# Vender

La pantalla **Venta** (Ctrl+1) tiene a la izquierda el **buscador** y el **carrito**, y a la derecha los **totales** y los botones.

Para vender, la caja debe estar abierta. Si está cerrada, la pantalla lo indica: pulsa **Abrir caja (F8)** y escribe el fondo inicial. Ver [Caja](ayuda:caja).

## Agregar productos

El cursor siempre vuelve al buscador (F2), así que se puede trabajar solo con el teclado o el lector de barras.

- **Con lector de código de barras**: escanea. El producto se agrega solo.
- **Por código**: escribe el código del producto y pulsa **Enter**.
- **Por nombre**: escribe parte del nombre (por ejemplo `arroz`). Aparece una lista con los que coinciden, su stock y su precio. Baja con **↓**, elige con **Enter** o haz clic. Si solo hay uno, Enter lo agrega directamente.
- **Con cantidad**: escribe la cantidad, un asterisco y el código o nombre: `3*7501055300`, `2.5*arroz`. Sin asterisco la cantidad es 1.

Si escaneas o agregas un producto que ya está en el carrito, se suma a la misma línea.

Los productos **desactivados** no aparecen. Si el stock no alcanza, la línea muestra *stock insuficiente* en amarillo; se puede seguir, y al cobrar el programa pide confirmación.

## Cambiar el carrito

Primero elige la línea: haz clic en ella o usa **Ctrl+↑ / Ctrl+↓** (o **↑ / ↓** desde el buscador vacío). La línea elegida se ve resaltada.

| Acción | Cómo |
|---|---|
| Sumar o restar 1 | Escribe **+** o **−** con el buscador vacío |
| Cambiar la cantidad | **F3**, doble clic en la línea o botón *Cantidad* |
| Quitar la línea | **Supr** con el buscador vacío, Ctrl+Supr o *Quitar línea* |
| Vaciar todo | **Esc** con el buscador vacío, o *Cancelar venta* (pide confirmación) |

En la ventana **Cantidad** se aceptan decimales para productos por peso o medida (`1.5` lb, `0.25` kg). Enter acepta, Esc cancela.

## Descuento (F7)

El descuento se aplica a **toda la venta**, no a una línea:

1. Pulsa **F7** o *Descuento*.
2. Elige **Importe** (por ejemplo `50.00`) o **Porcentaje** (por ejemplo `10`). **Tab** cambia entre los dos.
3. **Enter** para aplicar. *Quitar descuento* lo elimina.

El descuento no puede ser mayor que el subtotal. A la derecha se ve el subtotal, el descuento y el **TOTAL** en la moneda base.

## Cobrar (F4)

Pulsa **F4** o **Cobrar**. Se abre la ventana de cobro: ver [Cobrar y dar vuelto](ayuda:cobro).

Al terminar, el carrito queda vacío y en el centro se muestra el número de la venta y el **vuelto** que hay que dar, para no olvidarlo. El mensaje desaparece al agregar el siguiente producto.

## Ventas recientes y reimprimir (F10)

**F10** muestra las últimas 50 ventas con la vista previa de su ticket. Desde ahí se puede reimprimir, volver a generar el PDF del día, o **devolver productos y anular la venta**. Ver [Tickets](ayuda:tickets) y [Devoluciones y anulaciones](ayuda:devoluciones).

## Atajos de esta pantalla

| Tecla | Acción |
|---|---|
| F2 | Ir al buscador |
| Enter | Agregar lo escrito |
| F3 | Cantidad de la línea elegida |
| + / − | Sumar o restar 1 a la línea elegida |
| Supr | Quitar la línea elegida |
| F7 | Descuento |
| F4 | Cobrar |
| F10 | Ventas recientes: reimprimir, devolver o anular |
| F8 | Abrir caja (si está cerrada) |
| Esc | Limpiar el buscador o cancelar la venta |
