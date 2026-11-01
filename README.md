# PosCuba

[![CI](https://github.com/AlbertoFeito/PosCuba/actions/workflows/ci.yml/badge.svg)](https://github.com/AlbertoFeito/PosCuba/actions/workflows/ci.yml)

Punto de venta de escritorio para negocios pequeños (bodegas, cafeterías, talleres, mipymes).
Funciona 100 % sin internet. C++ / Qt Quick (QML) / SQLite. Compila con **Qt 5.14+ y Qt 6.x**.

## Compilar

### Qt Creator
Abrir `CMakeLists.txt` como proyecto, elegir el kit (MinGW o MSVC) y compilar. El ejecutable se llama `PosCuba.exe`.

### PowerShell
```powershell
# Qt 6 + MinGW (ajusta las rutas a tu instalación)
.\scripts\build.ps1 -QtDir C:\Qt\6.8.3\mingw_64 -MingwBin C:\Qt\Tools\mingw1310_64\bin -Test -Run

# Qt 5.14 + MinGW
.\scripts\build.ps1 -QtDir C:\Qt\5.14.2\mingw73_64 -MingwBin C:\Qt\Tools\mingw730_64\bin -Test -Run
```
Opciones: `-Config Release`, `-Clean`, `-Test` (ejecuta los tests), `-Run`, `-Software` (render por software).

### Distribuir
```powershell
.\scripts\deploy.ps1 -QtDir C:\Qt\6.8.3\mingw_64 -MingwBin C:\Qt\Tools\mingw1310_64\bin
```
Genera `dist\PosCuba\` (Release, sin consola, con `windeployqt`) y un `.zip`. Se copia la carpeta a la otra PC y se abre `PosCuba.exe`. No requiere instalar nada.
Si en una PC vieja la ventana sale en negro, usar `PosCuba (modo compatible).bat`.

## Primer uso
1. Crear el PIN del **Administrador** (4 a 8 dígitos).
2. **Configuración** (Ctrl+6): datos del negocio, salida del ticket (PDF o impresora), monedas y tasas.
3. **Usuarios** (Ctrl+7): crear los cajeros con su PIN.
4. **Productos** (Ctrl+2) e **Inventario** (Ctrl+3): cargar productos y registrar las entradas de mercancía.
5. **Venta** (Ctrl+1): abrir caja con el fondo inicial y vender.

## Atajos
| Pantalla | Teclas |
|---|---|
| General | Ctrl+1…8 pantallas · Ctrl+L cerrar sesión · Ctrl+Q salir |
| Venta | F2 buscar · `3*código` cantidad · F3 cantidad · `+`/`−` ajustar · Supr quitar línea · F7 descuento · F4 cobrar · F10 reimprimir · F8 abrir caja · Esc cancelar |
| Cobro | Enter confirmar · F6 cambiar moneda del efectivo · Tab siguiente campo · Esc volver |
| Productos | Ctrl+N nuevo · F2 buscar · Enter/F3 editar · Supr desactivar · Ctrl+S guardar |
| Inventario | F5 entrada · F6 ajuste |
| Caja | F8 abrir · F9 cerrar (arqueo) |
| Respaldos | Ctrl+B respaldar ahora |

## Roles
- **Administrador**: todo.
- **Cajero**: Venta y Caja (abrir, cerrar, ingresos y retiros, reimprimir).

Los permisos se comprueban también en el código C++, no solo en la interfaz.

## Dónde están los datos
| Qué | Dónde |
|---|---|
| Base de datos | `%APPDATA%\PosCuba\PosCuba\pos.db` |
| Respaldos | `Documentos\PosCuba\Respaldos` (o la USB elegida: `X:\PosCuba_Respaldos`) |
| Tickets PDF | `Documentos\PosCuba\Tickets\tickets_AAAA-MM-DD.pdf`: un PDF por día, cada venta en hoja nueva |
| Reportes CSV | `Documentos\PosCuba\Reportes` |

Respaldo automático una vez al día y al cerrar caja; se conservan los últimos 30. Los manuales nunca se borran.

## Decisiones técnicas
- **Dinero** en enteros (centavos) y **cantidades** en milésimas: sin errores de redondeo con decimales.
- **Venta en una transacción**: venta, líneas, pagos y descuento de stock se guardan juntos o no se guarda nada.
- **Stock negativo**: solo con confirmación explícita.
- **Monedas**: precios en la moneda base; otras monedas se aceptan en efectivo con su tasa. Cada pago guarda la tasa usada.
- **PIN**: SHA-256 con sal por usuario; bloqueo de 30 s tras 5 fallos.
- **Migraciones** versionadas (`PRAGMA user_version`); los respaldos viejos se migran al restaurar.
- **Ticket**: texto monoespaciado de 32/48 columnas impreso con `QPrinter` (cualquier impresora con driver de Windows) o PDF diario (una venta por hoja; un ticket largo continúa en hojas de hasta 297 mm).

## Estructura
```
src/core        dominio y servicios (sin QML)   src/data        SQLite: conexión, migraciones, repositorios
src/models      modelos de lista para QML        src/controllers objetos expuestos a QML
src/printing    impresión de tickets             qml/            pantallas, diálogos, tema
tests/          QtTest (11 suites)               scripts/        build.ps1, deploy.ps1
```

## Tests
`.\scripts\build.ps1 ... -Test` o `ctest` en la carpeta de compilación. Cubren dinero, productos, inventario, cálculo de venta y vuelto, venta completa y cierre de caja, ticket, impresión a PDF, reportes, usuarios, respaldos y monedas.
