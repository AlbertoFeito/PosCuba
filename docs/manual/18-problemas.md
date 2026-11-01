# Solución de problemas

## La ventana sale en negro, en blanco o parpadea

Pasa en PCs viejas con drivers de video antiguos. Abre el programa con **PosCuba (modo compatible).bat**, que está en la misma carpeta. Usa dibujo por software: es un poco más lento pero funciona en cualquier PC.

## No puedo vender: «La caja está cerrada»

Abre la caja con **F8** y el fondo inicial. Ver [Caja](ayuda:caja).

## «No existe un producto con el código …»

- Revisa que el código esté bien escrito.
- El producto puede estar **desactivado**: búscalo en Productos con *Mostrar inactivos*.
- Si es un código de barras nuevo, agrégalo al producto en Productos → Editar → *Código de barras*.

## El lector de barras no hace nada

- Comprueba que el cursor esté en el buscador (**F2**).
- Abre el Bloc de notas y escanea: si no aparece el número, el problema es del lector o del cable.

## «PIN incorrecto. Usuario bloqueado 30 segundos»

Tras 5 errores seguidos hay que esperar 30 segundos. Si el usuario olvidó el PIN, ver [Inicio de sesión y PIN](ayuda:sesion).

## Me equivoqué en una venta

No se borra: haz una devolución o anúlala desde **F10 → Devolver / anular…**. Ver [Devoluciones y anulaciones](ayuda:devoluciones).

## No se genera el PDF del ticket

El PDF del día está abierto en otro programa (visor de PDF, navegador). Ciérralo y, en Venta, pulsa **F10 → PDF del día**. La venta ya está guardada.

## El ticket sale cortado o con márgenes

En Windows: Panel de control → Dispositivos e impresoras → clic derecho en la impresora → *Preferencias de impresión* → tamaño de papel del rollo (58 u 80 mm). Comprueba también el ancho en [Configuración](ayuda:configuracion).

## La impresora no aparece en la lista

Instala su driver de Windows, enciéndela y pulsa **Actualizar** junto a la lista de impresoras.

## Un producto tiene stock negativo

Se vendió más de lo registrado. Corrígelo con una entrada o un ajuste en [Inventario](ayuda:inventario).

## La caja no cuadra al cerrar

- ¿Se registraron todos los retiros (pagos a proveedores, gastos)?
- ¿Algún pago por transferencia se cobró como efectivo, o al revés?
- ¿El vuelto de un pago en USD/MLC se dio bien? El vuelto siempre es en la moneda base.
- ¿El fondo inicial se contó bien al abrir?
- ¿Se entregó al cliente el dinero de una devolución en efectivo? Aparece en el recuadro *Devoluciones*.

## La ganancia sale muy alta

Hay productos sin **precio de costo**: toda su venta cuenta como ganancia. Carga los costos en Productos o al registrar las entradas.

## El CSV se abre en una sola columna en Excel

Expórtalo con el otro formato (*Excel en español* o *Estándar*). Ver [Reportes](ayuda:reportes).

## «El respaldo es de una versión más nueva del programa»

Ese respaldo se hizo con un PosCuba más nuevo. Actualiza el programa en esta PC y vuelve a restaurar.

## «No se pudo restaurar; los datos actuales no cambiaron»

El respaldo está dañado o incompleto. Prueba con otro más antiguo. Tus datos actuales no se tocaron.

## La pantalla Estado (Ctrl+9)

Muestra dónde está la base de datos, su versión, la de SQLite y la de Qt. Si el programa no puede abrir la base, se abre directamente esta pantalla con el error: anótalo para pedir soporte.

## Dónde están los datos

| Qué | Dónde |
|---|---|
| Base de datos | `%APPDATA%\PosCuba\PosCuba\pos.db` |
| Respaldos | `Documentos\PosCuba\Respaldos` (o `X:\PosCuba_Respaldos` en la USB) |
| Tickets PDF | `Documentos\PosCuba\Tickets` |
| Reportes CSV | `Documentos\PosCuba\Reportes` |
| Productos exportados y plantilla | `Documentos\PosCuba\Productos` |
| Este manual en PDF | `Documentos\PosCuba\Manual_PosCuba.pdf` |

No copies ni borres `pos.db` con el programa abierto. Para copias de seguridad usa siempre [Respaldos](ayuda:respaldos).
