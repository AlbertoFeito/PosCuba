# Usuarios y permisos

La pantalla **Usuarios** (Ctrl+7, solo administrador) gestiona quién puede entrar.

## Roles

| Rol | Puede |
|---|---|
| **Administrador** | Todo: productos, inventario, reportes, configuración, usuarios y respaldos, además de vender y manejar la caja |
| **Cajero** | Solo **Venta** y **Caja**: vender, reimprimir, abrir y cerrar caja, ingresos y retiros |

Los permisos se comprueban también por dentro del programa, no solo ocultando botones.

## La lista

Muestra nombre (el tuyo marcado con *(tú)*), rol, estado (*Activo*, *Sin PIN* o *Inactivo*) y la fecha del último ingreso.

## Crear un usuario (Ctrl+N)

1. **Nombre**: único, hasta 40 caracteres.
2. **Rol**: Administrador o Cajero.
3. **PIN** y **Repetir PIN**: de 4 a 8 dígitos.
4. **Guardar (Enter)**.

## Editar, asignar PIN, desactivar

- **Editar** (Enter o doble clic): cambia nombre y rol.
- **Asignar PIN**: pone un PIN nuevo sin conocer el anterior. Úsalo cuando alguien lo olvida.
- **Desactivar** (Supr): el usuario ya no puede entrar, pero sus ventas y movimientos siguen a su nombre. *Reactivar* lo devuelve.

Reglas que el programa protege:

- No puedes desactivarte a ti mismo.
- Siempre debe quedar **al menos un administrador activo**.

## Recomendaciones

- Un usuario por persona; no compartan PIN. Así cada venta y cada cierre de caja tiene responsable.
- Ten **dos administradores**: si uno olvida su PIN, el otro se lo restablece. Ver [Inicio de sesión y PIN](ayuda:sesion).
- Cuando alguien deja de trabajar, desactívalo.
