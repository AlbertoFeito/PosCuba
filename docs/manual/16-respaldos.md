# Respaldos y restauración

Un **respaldo** es una copia completa de todos los datos: productos, inventario, ventas, caja, usuarios y configuración. Si la PC se rompe, se la roban o se daña el disco, con un respaldo reciente no se pierde nada.

La pantalla **Respaldos** (Ctrl+8) es solo para administradores.

## Hacer un respaldo

1. Elige **dónde** guardarlo (ver abajo).
2. Pulsa **Respaldar ahora (Ctrl+B)**.

El archivo se llama `PosCuba_respaldo_AAAA-MM-DD_HHMMSS.db` (los automáticos, `PosCuba_auto_…`) y se comprueba al crearlo. Se puede hacer en cualquier momento, incluso con la caja abierta.

## Dónde guardarlo

- **Carpeta de respaldos**: `Documentos\PosCuba\Respaldos` en la propia PC.
- **Memorias USB y otros discos**: aparece un botón por unidad con su espacio libre. Al elegirla, los respaldos van a `X:\PosCuba_Respaldos`. Si conectaste la USB después de abrir la pantalla, pulsa **Actualizar unidades**.
- **Otra carpeta**: escríbela y pulsa *Usar carpeta*.

La lista muestra los respaldos de la carpeta elegida: fecha, tipo y tamaño. **Abrir carpeta** la abre en Windows.

> Guarda un respaldo en una USB **al menos una vez por semana** y llévala fuera del negocio. Un respaldo en la misma PC no sirve si se pierde la PC.

## Respaldo automático

Si está activado (*Al iniciar cada día y al cerrar caja*), el programa hace un respaldo:

- la primera vez que se abre cada día, y
- cada vez que se cierra la caja.

Van a la carpeta de respaldos de la PC. Se conservan los **últimos 30** automáticos (se puede cambiar de 1 a 365); los más viejos se borran solos. Los **manuales nunca se borran**. Pulsa **Guardar** después de cambiar estas opciones.

| Tipo | Origen |
|---|---|
| Manual | Respaldar ahora |
| Automático | Inicio del día o cierre de caja |
| Antes de restaurar | Copia de seguridad hecha justo antes de una restauración |

## Restaurar

**Restaurar reemplaza todos los datos actuales** por los del respaldo.

1. Elige el respaldo en la lista. A la derecha se ven sus datos: fecha, cantidad de productos y ventas, y la última venta. Comprueba que es el que buscas.
2. Pulsa **Restaurar este respaldo…**
3. Escribe **RESTAURAR** y confirma.

Qué pasa:

- Antes de restaurar se guarda automáticamente una copia de los datos actuales (tipo *Antes de restaurar*). Si te equivocaste, restaura esa copia.
- Se cierra la sesión: entra con los usuarios y PIN **del respaldo**.
- Un respaldo de una versión anterior del programa se actualiza solo.
- Si el respaldo está dañado o no es de PosCuba, no se restaura y los datos actuales quedan intactos.

## Pasar el negocio a otra PC

1. En la PC vieja: respaldo a una USB.
2. En la PC nueva: copia la carpeta del programa, ábrelo y crea un PIN cualquiera para el Administrador.
3. Respaldos → elige la USB → elige el respaldo → Restaurar.
4. Entra con los usuarios y PIN de siempre.
