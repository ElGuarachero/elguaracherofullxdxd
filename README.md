# EL MAESTRO SAICHAN

Plugin VST3 (Windows x64) de ducking/pumping disparado por el kick, pensado para FL Studio.

## Parámetros

- **Depth**: cuánto baja el volumen al entrar el kick (0–100 %).
- **Threshold**: nivel a partir del cual el kick dispara el efecto (-60 a 0 dB).
- **Attack**: velocidad de bajada de ganancia (0.1–50 ms).
- **Hold**: tiempo que se mantiene el ducking tras el kick (0–500 ms).
- **Release**: velocidad de recuperación (5–1000 ms).
- **Mix**: mezcla seca/húmeda (0–100 %).
- **External Sidechain**: si está activo y el host envía sidechain, el disparo sale de esa entrada (el kick) en lugar de la señal principal.

## Subir a GitHub y compilar

1. Crea un repositorio nuevo en tu cuenta de GitHub (por ejemplo `el-maestro-saichan`).
2. Sube el contenido de esta carpeta (incluida la carpeta oculta `.github`):

   ```bash
   cd EL-MAESTRO-SAICHAN
   git init
   git add .
   git commit -m "EL MAESTRO SAICHAN v1.0"
   git branch -M main
   git remote add origin https://github.com/TU_USUARIO/el-maestro-saichan.git
   git push -u origin main
   ```

3. Ve a la pestaña **Actions** del repositorio. Se ejecutará *Build VST3 (Windows)*.
4. Cuando termine, entra en la ejecución y descarga el artefacto **EL-MAESTRO-SAICHAN-VST3**. Dentro está `EL MAESTRO SAICHAN.vst3`.

También puedes lanzarlo a mano con **Run workflow**.

## Instalar en FL Studio (Windows)

1. Copia `EL MAESTRO SAICHAN.vst3` a `C:\Program Files\Common Files\VST3\`.
2. En FL Studio: *Options → Manage plugins → Find installed plugins*.
3. Carga el plugin en un mixer insert. Para sidechain: en FL, usa el enrutamiento de sidechain del mixer hacia la entrada secundaria del plugin y activa **External Sidechain**.

## Compilar en local (opcional)

Requiere Visual Studio 2022 con C++ y CMake:

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target ElMaestroSaichan_VST3
```
