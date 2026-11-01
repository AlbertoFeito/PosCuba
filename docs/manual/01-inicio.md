# Primeros pasos

PosCuba es un punto de venta para negocios pequeños: bodegas, cafeterías, puntos de venta de mipymes, talleres. Funciona **sin internet**: todos los datos quedan en la propia computadora.

## Qué necesitas

- Una PC con Windows 10 u 11 y pantalla de 1366×768 o mayor. (Para Windows 7 hace falta la versión compilada con Qt 5.)
- Opcional: un lector de código de barras USB (funciona como un teclado; no necesita configuración).
- Opcional: una impresora térmica de tickets de 58 u 80 mm con su driver de Windows instalado. Sin impresora, los tickets se guardan en PDF.

## Instalar

**Con el instalador** (`PosCuba-<versión>-Setup.exe`):

1. Ábrelo y sigue los pasos. No pide permisos de administrador: instala para tu usuario (puedes elegir «para todos los usuarios»).
2. Marca *Crear un acceso directo en el escritorio* si lo quieres.
3. En el menú Inicio quedan **PosCuba**, **PosCuba (modo compatible)** y **Desinstalar PosCuba**.

Para **actualizar** a una versión nueva, ejecuta el instalador nuevo encima: los datos se conservan. Desinstalar tampoco borra los datos ni los respaldos.

**Sin instalador** (carpeta o .zip):

1. Copia la carpeta **PosCuba** a la PC (por ejemplo a `C:\PosCuba`).
2. Abre **PosCuba.exe**. Si quieres, crea un acceso directo en el escritorio (clic derecho → Enviar a → Escritorio).

Si la ventana sale en negro o parpadea (pasa en PCs viejas), usa **PosCuba (modo compatible)**. Ver [Solución de problemas](ayuda:problemas).

## La primera vez

Al abrir el programa por primera vez te pide crear el **PIN del Administrador** (de 4 a 8 dígitos). Ese usuario puede hacer todo: no olvides el PIN. Ver [Inicio de sesión y PIN](ayuda:sesion).

Después, en este orden:

1. **Configuración** (Ctrl+6): nombre, dirección y teléfono del negocio, y cómo sale el ticket. Ver [Configuración](ayuda:configuracion).
2. **Monedas** (pestaña *Monedas y tasas* de Configuración): si aceptas USD, MLC u otra moneda en efectivo, agrégala con su tasa. Ver [Monedas y tasas](ayuda:monedas).
3. **Usuarios** (Ctrl+7): crea un usuario para cada cajero, con su PIN. Ver [Usuarios](ayuda:usuarios).
4. **Productos** (Ctrl+2): crea el catálogo con precios y costos, a mano o desde un Excel. Ver [Productos](ayuda:productos) e [Importar productos](ayuda:importar).
5. **Inventario** (Ctrl+3): registra la mercancía que tienes con una **entrada** por producto. Ver [Inventario](ayuda:inventario).
6. **Respaldos** (Ctrl+8): haz un primer respaldo en una memoria USB. Ver [Respaldos](ayuda:respaldos).

## El día a día

1. El cajero entra con su PIN.
2. **Abre la caja** con el dinero que hay en la gaveta para dar vuelto (F8). Ver [Caja](ayuda:caja).
3. **Vende** (Ctrl+1): escanea o escribe el código, cobra con F4. Ver [Vender](ayuda:venta) y [Cobrar](ayuda:cobro).
4. Si sale o entra dinero que no es una venta (pago a un proveedor, cambio adicional), regístralo en **Caja** como retiro o ingreso.
5. Al terminar, **cierra la caja** (F9): cuenta el dinero y el programa te dice si sobra o falta.
6. Cuando llega mercancía, el administrador registra la **entrada** en Inventario.

## La pantalla

- **Barra izquierda**: las pantallas del programa. Cada una tiene su atajo (Ctrl+1 a Ctrl+9). El cajero solo ve *Venta* y *Caja*.
- **Abajo a la izquierda**: tu nombre y rol, *Cambiar mi PIN*, *Cerrar sesión*, *Ayuda* y el tema oscuro.
- **Mensajes**: los avisos aparecen abajo unos segundos; en verde si todo salió bien, en rojo si hubo un problema.

## Ayuda dentro del programa

- **F1** abre este manual en la sección de la pantalla o ventana que tienes delante.
- Si dejas el mouse quieto sobre un botón o campo, aparece una **explicación corta**. Se pueden apagar con la casilla *Mostrar ayudas al pasar el mouse*, arriba en este manual.
- El buscador de la izquierda encuentra palabras en todo el manual, con o sin tildes.
- **Guardar en PDF** crea `Documentos\PosCuba\Manual_PosCuba.pdf` para imprimirlo.
