# Productos y categorías

La pantalla **Productos** (Ctrl+2, solo administrador) es el catálogo de lo que vendes.

## Buscar

- Escribe en el buscador (F2) parte del nombre, el código o el código de barras. La lista se filtra al momento.
- Con un lector de barras, escanea: si hay un solo resultado, se abre para editarlo.
- **Categoría**: muestra solo los de una categoría.
- **Mostrar inactivos**: incluye los productos desactivados (aparecen atenuados).
- **↓** pasa del buscador a la lista; **Esc** vuelve al buscador.

Los productos con stock igual o menor que su **stock mínimo** se ven en rojo.

## Crear un producto (Ctrl+N)

| Campo | Qué poner |
|---|---|
| Nombre * | Como quieres que salga en el ticket. Hasta 120 caracteres |
| Código * | Código interno, el que se escribe al vender. Sin espacios, hasta 32 caracteres. Debe ser único |
| Código de barras | Opcional. Escanéalo con el lector. Debe ser único |
| Categoría | Opcional. **+** crea una categoría nueva |
| Unidad | u (unidad), kg, lb, l (litro) o m (metro) |
| Precio de costo | Lo que te cuesta a ti. Se usa para calcular la ganancia |
| Precio de venta * | El precio al cliente, en la moneda base |
| Stock mínimo | Cantidad a partir de la cual el producto se marca como "stock bajo". Vacío = sin alerta |

Los campos con **\*** son obligatorios. **Ctrl+S** o **Enter** guarda; **Esc** cancela.

### El botón Generar

Al crear un producto, el **Código** ya viene lleno con el **siguiente número libre**: el mayor código numérico existente más 1 (si tienes `1`, `2` y `15`, propone `16`). Los códigos con letras no cuentan. **Generar** vuelve a poner ese número si lo cambiaste. Puedes usar cualquier otro código, por ejemplo `REF-01`.

### Código y código de barras

- Al vender se puede usar **cualquiera de los dos**.
- El lector de barras envía Enter al final: en esta ventana eso pasa al siguiente campo, no guarda.

### Ganancia

Debajo del formulario se muestra la **ganancia por unidad** y su porcentaje sobre el costo. Si el precio es menor que el costo, aparece un aviso de pérdida.

## Editar

Doble clic, **Enter** o **F3** sobre el producto, o el botón *Editar*. El cambio de precio vale para las ventas siguientes; las ventas ya hechas conservan su precio.

El **stock no se cambia aquí**: se hace con entradas y ajustes en [Inventario](ayuda:inventario), para que quede el historial.

## Desactivar en lugar de borrar

Los productos no se borran, porque ya pueden tener ventas. **Supr** o *Desactivar* lo esconde de la venta y de las búsquedas. Para recuperarlo, marca *Mostrar inactivos*, elígelo y pulsa *Reactivar*.

## Muchos productos a la vez

- **Cambiar precios…**: sube o baja precios en lote, por porcentaje y con redondeo. Ver [Cambiar precios en lote](ayuda:precios).
- **Importar / exportar…**: carga productos desde Excel (CSV) o guarda el catálogo en un archivo. Ver [Importar y exportar productos](ayuda:importar).

## Categorías

Sirven para ordenar y filtrar. Se crean desde la ventana del producto con el botón **+** (hasta 60 caracteres).

## Mensajes frecuentes

| Mensaje | Qué hacer |
|---|---|
| Ya existe un producto con el código … | Usa otro código o *Generar* |
| Ya existe un producto con el código de barras … | Ese código de barras ya está en otro producto (búscalo, puede estar inactivo) |
| El código no puede tener espacios… | Quita los espacios: `REF01` o `REF-01` |
