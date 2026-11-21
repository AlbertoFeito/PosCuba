# Gastos y ganancia neta

La pantalla **Gastos** (Ctrl+Shift+G, solo administrador) registra lo que cuesta mantener el negocio: alquiler, electricidad, salarios, transporte, impuestos… Con esos datos [Reportes](ayuda:reportes) calcula la **ganancia neta**: lo que de verdad queda después de pagar todo.

## La pantalla

- Arriba, el período: **Este mes**, **Mes anterior**, **30 días**, **Este año** o un rango libre `dd/mm/aaaa` con *Aplicar*.
- A la izquierda, el **total del período** y cuánto se gastó **por categoría**, con una barra de proporción.
- A la derecha, la lista: fecha, categoría, descripción, importe y cómo se pagó:
  - **Caja**: salió de la gaveta.
  - **Fuera**: se pagó con otro dinero (cuenta del banco, bolsillo del dueño).
  - **Fijo**: lo registró solo un gasto fijo mensual.
- La lista de la derecha (arriba) filtra por categoría.

## Registrar un gasto (Ctrl+N)

1. **Nuevo gasto (Ctrl+N)**.
2. Elige la **categoría**. Para una nueva, pulsa **+**, escribe el nombre y **Enter**.
3. Escribe la **descripción** (ej.: *Corriente de noviembre*) y el **importe**.
4. La **fecha** viene con hoy; se puede poner una anterior, nunca una futura.
5. **Pagado con dinero de la caja**: márcalo si el dinero sale de la gaveta.
6. **Ctrl+S** o *Guardar*.

Un gasto **pagado con dinero de la caja**:

- necesita la caja abierta y la fecha de hoy;
- crea un **retiro** en la [Caja](ayuda:caja) con el texto *Gasto · categoría: descripción*, que baja el efectivo esperado;
- no se puede si en la gaveta no hay ese dinero.

## Gastos fijos mensuales

Para gastos que se repiten cada mes (alquiler, salario, cuota):

1. Pulsa **Gastos fijos…**.
2. Abajo, elige categoría, escribe la descripción, el importe y el **día del mes** (1 a 31).
3. **Agregar**.

Desde ese mes, el día indicado el programa registra el gasto solo, como pagado **fuera** de la caja. Si el mes es más corto (día 31 en febrero), se registra el último día. Si el programa no se abrió ese día, lo registra la próxima vez que se abra.

En la misma ventana se puede **pausar** un gasto fijo (deja de registrarse) y **reanudarlo**. Al reanudarlo no se registran los meses que estuvo pausado.

## Borrar un gasto

Elige el gasto y pulsa **Supr** o **Borrar gasto**. Un gasto pagado de la caja solo se puede borrar **mientras esa caja siga abierta**; se borra también su retiro. Después del cierre ya forma parte del arqueo y no se puede borrar.

## Categorías

Vienen creadas: Alquiler, Electricidad, Agua y gas, Salarios, Transporte, Impuestos y tasas, Mantenimiento, Compras menores y Otros. Se agregan más con **+** al registrar un gasto.

La **compra de mercancía para vender no es un gasto**: se registra como entrada en [Inventario](ayuda:inventario) y su costo ya se descuenta en la ganancia bruta. Registrarla también aquí la contaría dos veces.

## Ganancia neta

En [Reportes](ayuda:reportes):

- **Ganancia bruta** = total vendido − costo de lo vendido.
- **Gastos** = gastos con fecha en el período.
- **Ganancia neta** = ganancia bruta − gastos. En rojo si es negativa.

La tabla de ventas por período tiene la columna **Gastos**, y el CSV de ventas por período incluye *Ganancia bruta*, *Gastos* y *Ganancia neta*.

## Exportar CSV

**Exportar CSV** guarda los gastos del período (fecha, categoría, descripción, importe, forma de pago) en `Documentos\PosCuba\Reportes`, para abrir en Excel.

## Mensajes frecuentes

| Mensaje | Qué hacer |
|---|---|
| Elige la categoría del gasto | Elige una en la lista o crea una con + |
| La fecha del gasto no puede ser futura | Pon la fecha de hoy o una anterior |
| Un gasto pagado con dinero de la caja tiene que ser de hoy | Desmarca *Pagado con dinero de la caja* o pon la fecha de hoy |
| No hay suficiente efectivo en caja | Desmarca la casilla si se pagó con otro dinero, o registra un ingreso antes |
| Este gasto salió de una caja que ya se cerró | El gasto ya está en un arqueo cerrado; registra un ingreso si fue un error |
