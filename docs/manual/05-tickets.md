# Tickets: PDF e impresora

Cada venta puede generar un **ticket** con los datos del negocio, el número de venta, la fecha, el cajero, los productos, el descuento, el total, las formas de pago y el vuelto. El formato es de rollo de 58 mm (32 caracteres por línea) u 80 mm (48 caracteres).

La salida se elige en [Configuración](ayuda:configuracion) → *Salida del ticket*.

## PDF diario (sin impresora)

Es la opción por defecto. Todos los tickets de un día van a **un solo PDF**:

`Documentos\PosCuba\Tickets\tickets_AAAA-MM-DD.pdf` (por ejemplo `tickets_2026-11-01.pdf`)

- Cada venta empieza en una **hoja nueva**, del tamaño del rollo.
- Si un ticket es muy largo, continúa en las hojas siguientes y la próxima venta vuelve a empezar en hoja nueva.
- El PDF se regenera desde los datos guardados cada vez que se agrega una venta, así que siempre está completo y en orden.
- Si el PDF del día está **abierto en un visor** mientras se cobra, Windows no deja reemplazarlo: la venta se guarda igual y aparece un aviso. Cierra el visor y pulsa *PDF del día* en Ventas recientes (F10).

Ese PDF se puede imprimir después, enviar por WhatsApp o guardar como comprobante.

## Impresora térmica

1. Instala el driver de Windows de la impresora y comprueba que imprime una página de prueba de Windows.
2. En Configuración elige **Impresora**, la impresora en la lista (o *Predeterminada del sistema*) y el ancho del papel.
3. Pulsa **Imprimir prueba**.

Si el ticket sale cortado, muy pequeño o con márgenes grandes, abre en Windows las *Preferencias de impresión* de la impresora y elige el tamaño de papel del rollo (58 u 80 mm). Si acabas de conectar la impresora, pulsa **Actualizar** junto a la lista.

## Generar el ticket al cobrar

La casilla de la ventana de cobro (*Agregar ticket al PDF del día* o *Imprimir ticket*) decide si sale el ticket de esa venta. Su valor inicial es la opción **Al cobrar** de la configuración. Se puede marcar o desmarcar en cada venta.

## Ventas recientes y reimpresión (F10)

En **Venta**, pulsa **F10** para ver las últimas 50 ventas:

- A la izquierda, la lista: número, fecha y hora, líneas, cajero y total.
- A la derecha, la vista previa del ticket exactamente como sale.
- **Imprimir (Enter)** o doble clic: lo envía a la impresora configurada.
- **PDF del día**: vuelve a generar el PDF del día de esa venta, con todas las ventas de ese día.
- **Abrir carpeta**: abre la carpeta de los PDF.

## Ticket de prueba

En Configuración, **PDF de prueba** e **Imprimir prueba** generan un ticket de ejemplo con los datos del negocio para revisar el formato antes de vender. La **vista previa** de la derecha cambia al momento mientras escribes.
