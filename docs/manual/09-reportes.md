# Reportes

La pantalla **Reportes** (Ctrl+5, solo administrador) resume las ventas de un período. Todos los importes están en la **moneda base**.

## Elegir el período

- Botones rápidos: **Hoy**, **Ayer**, **Esta semana** (desde el lunes), **Este mes**, **Mes anterior**, **30 días** y **Este año**.
- Rango libre: escribe las fechas **desde** y **hasta** con el formato `dd/mm/aaaa` y pulsa **Enter** o *Aplicar*. El rango puede ser de hasta 10 años.

## Resumen

| Recuadro | Qué es |
|---|---|
| Ventas | Cantidad de ventas |
| Total vendido | Suma de los totales (debajo, los descuentos dados) |
| Ganancia | Total vendido menos el costo de lo vendido; debajo, el margen en % |
| Ticket promedio | Total vendido ÷ cantidad de ventas |
| Efectivo | Cobrado en efectivo, convertido a la moneda base y sin el vuelto |
| Transferencias | Cobrado por transferencia |

La **ganancia** usa el costo que tenía cada producto **en el momento de la venta**. Si un producto no tiene costo, toda su venta cuenta como ganancia: carga los costos para que este número sea real.

## Ventas por período

El gráfico de barras muestra el total por **Día**, **Semana** o **Mes** (botones a la derecha). Deja el mouse sobre una barra para ver el total, la cantidad de ventas y la ganancia de ese período.

En rangos largos el programa agrupa solo: por semana si son más de 2 meses y por mes si son más de 2 años. Lo indica arriba del gráfico.

Debajo, la tabla con los mismos datos. Los períodos sin ventas aparecen atenuados.

## Productos más vendidos

Lista de productos vendidos en el período con cantidad, importe y ganancia. Se puede ordenar por **Cantidad**, **Importe** o **Ganancia**.

Los importes por producto son **antes** de los descuentos aplicados a toda la venta, por eso su suma puede ser mayor que el total vendido.

## Exportar a Excel (CSV)

Abajo, **Exportar CSV**:

- **Ventas por período**: la tabla del gráfico.
- **Productos**: los más vendidos.
- **Detalle de ventas**: una fila por venta, con cajero, subtotal, descuento, total, costo, ganancia, efectivo y transferencia.

Los archivos se guardan en `Documentos\PosCuba\Reportes`; **Abrir carpeta** la muestra.

**Formato**:

- **Excel en español (;)**: separador punto y coma y coma decimal. Es el que abre bien con doble clic en un Excel configurado en español.
- **Estándar (,)**: separador coma y punto decimal. Para Excel en inglés, LibreOffice o Google Sheets.

Si al abrir el CSV todo sale en una sola columna, exporta con el otro formato.
