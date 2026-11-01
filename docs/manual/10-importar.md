# Importar y exportar productos

**Importar / exportar…** (en **Productos**, Ctrl+I, solo administrador) carga productos desde un archivo de **Excel guardado como CSV**, o guarda todo el catálogo en uno. Sirve para:

- Cargar el catálogo inicial sin escribirlo producto por producto.
- Actualizar muchos precios o costos desde una lista del proveedor.
- Pasar el catálogo a otra PC o a otro negocio.
- Revisar o imprimir la lista de precios en Excel.

## Exportar

1. Pestaña **Exportar**.
2. **Formato**: *Excel en español (;)* para abrirlo con doble clic en un Excel en español; *Estándar (,)* para Excel en inglés, LibreOffice o Google Sheets.
3. Opcional: **Incluir productos inactivos**.
4. **Exportar productos**. Se guarda en `Documentos\PosCuba\Productos\productos_AAAA-MM-DD.csv` y se abre la carpeta.

## Preparar el archivo para importar

La forma más fácil es pulsar **Guardar plantilla** (pestaña *Importar*): crea `plantilla_productos.csv` con las columnas y dos ejemplos. Ábrela en Excel, borra los ejemplos y llénala.

| Columna | Qué poner | ¿Obligatoria? |
|---|---|---|
| Código | Código interno. Si un producto ya existe con ese código, se actualiza | No: vacío = número automático |
| Código de barras | El que trae el envase | No |
| Nombre | Nombre del producto | Sí, en los nuevos |
| Categoría | Si no existe, se crea | No |
| Unidad | u, kg, lb, l o m (también «unidad», «libra», «litro»…) | No (u) |
| Costo | Precio de costo | No |
| Precio | Precio de venta | Sí, en los nuevos |
| Stock mínimo | Para el aviso de stock bajo | No |
| Stock | Cantidad que hay | No |
| Activo | sí / no | No |

- Los encabezados se reconocen con o sin tildes y con otros nombres comunes (*Descripción*, *PVP*, *Existencia*…). Las columnas que no se reconocen se ignoran.
- Los números pueden tener coma o punto decimal: `1.234,50`, `1234.5`, `$ 120`.
- En Excel: **Archivo → Guardar como → CSV (delimitado por comas)** o **CSV UTF-8**. El programa entiende los dos.

## Importar

1. Pestaña **Importar desde Excel (CSV)**.
2. Elige el archivo en la lista (busca en *Documentos\PosCuba\Productos*, Documentos, Descargas, Escritorio y memorias USB) o escribe su ruta. Si acabas de conectar la USB, pulsa **Actualizar**.
3. **Revisar archivo**. Todavía no se guarda nada: se muestra cada fila con su estado.

| Estado | Significa |
|---|---|
| **Nuevo** | Se va a crear. Si trae stock, se registra como entrada de mercancía |
| **Cambia** | Ya existe; se muestra qué cambia (precio 120.00 → 130.00…) |
| **Igual** | Ya existe y no cambia nada |
| **Error** | No se importa; dice por qué (sin precio, unidad no válida, código repetido…) |

4. **Importar N productos**. Todo se guarda junto: si algo falla, no se guarda nada.

### Las columnas que faltan no se tocan

Si el archivo solo tiene *Código* y *Precio*, solo se cambian los precios: el nombre, el costo y lo demás quedan como estaban. Así se puede actualizar una lista de precios sin tocar nada más. Una **celda vacía** tampoco cambia nada.

Revisa los productos marcados **Cambia**: si un código del archivo ya es de otro producto, se le cambiaría el nombre (la revisión lo muestra como *nombre … → …*).

### Stock de los productos que ya existen

Sin marcar nada, la columna **Stock** solo se usa para los productos **nuevos**. Si marcas **Ajustar el stock de los productos existentes**, también corrige el stock de los que ya existen: queda como un **ajuste** con la nota *Importación de productos* en el historial de [Inventario](ayuda:inventario).

## Si algo sale mal

| Problema | Qué hacer |
|---|---|
| «No se reconoce el encabezado» | La primera fila debe tener los nombres de las columnas. Usa la plantilla |
| Todo sale en una sola columna en Excel | Exporta con el otro formato |
| «No se pudo abrir… ¿Está abierto en Excel?» | Cierra el archivo en Excel y vuelve a revisar |

Ver también: [Productos](ayuda:productos) y [Cambiar precios en lote](ayuda:precios).
