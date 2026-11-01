# Cambiar precios en lote

Con la inflación, cambiar los precios uno por uno lleva mucho tiempo. **Cambiar precios…** (en **Productos**, solo administrador) cambia muchos a la vez y muestra la lista antes de aplicar.

## Pasos

1. En **Productos**, pulsa **Cambiar precios…** (Ctrl+P).
2. **Productos**: todos, o solo una categoría. Marca **Incluir inactivos** si también quieres cambiar los desactivados.
3. **Cambio**: elige una de las dos formas (ver abajo).
4. Escribe el **porcentaje**: `10`, `12,5` o `-5` para bajar.
5. **Redondeo**: elige si los precios nuevos se redondean y cómo.
6. Revisa la lista: precio actual, precio nuevo y diferencia de cada producto.
7. **Aplicar a N productos** y confirma.

## Las dos formas

| Forma | Precio nuevo | Ejemplo |
|---|---|---|
| **% sobre el precio actual** | precio actual + el % | 120.00 con +10 % = 132.00 |
| **Costo + % de ganancia** | costo + el % | costo 80.00 con 50 % = 120.00 |

Con **Costo + %**, los productos **sin costo** cargado no se tocan; la lista dice cuántos son.

## Redondeo

Deja precios fáciles de cobrar y de dar vuelto.

| Redondeo | 132.40 hacia arriba | 132.40 al más cercano |
|---|---|---|
| Sin redondeo | 132.40 | 132.40 |
| a 1 | 133.00 | 132.00 |
| a 5 | 135.00 | 130.00 |
| a 10 | 140.00 | 130.00 |
| a 50 | 150.00 | 150.00 |
| a 100 | 200.00 | 100.00 |

**Hacia arriba** nunca deja un precio por debajo del cálculo; es lo más seguro para no perder margen.

## A tener en cuenta

- En la lista, un precio nuevo **en rojo** queda por debajo del costo: perderías dinero con ese producto.
- Los productos cuyo precio no cambia no aparecen en la lista.
- Las ventas ya hechas **conservan su precio**.
- No hay "deshacer": para volver atrás, aplica el cambio contrario. Antes de un cambio grande, haz un [respaldo](ayuda:respaldos) o [exporta los productos](ayuda:importar).
- Para cambiar el precio de un solo producto, edítalo en [Productos](ayuda:productos).
