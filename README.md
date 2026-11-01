# 🦾 Control y Simulador 3D — Taller de Automatización 7° AÑO

Panel de control interactivo y **Gemelo Digital 3D en tiempo real** para un brazo robótico impreso en 3D de **4 grados de libertad (4 DoF — EEZYbotARM)**, controlado mediante un **Arduino Nano** y 4 micro-servomotores **SG90 / MG90S** de 180°.

El sistema funciona de manera **100% offline** desde cualquier navegador compatible con Web Serial (Google Chrome o Microsoft Edge), manteniendo sincronizados ("hermanados") en todo momento el modelo 3D en pantalla y el brazo físico real.

---

## ✨ Características del Panel de Control y Gemelo Digital 3D (`panel_control.html`)

### 📐 1. Gemelo Digital 3D con Piezas STL Originales y Cinemática Exacta
- **13 Mallas CAD Oficiales**: Renderizado WebGL en tiempo real de las 13 piezas originales del brazo impreso en 3D más los 3 servomotores SG90 visibles, arandelas retenedoras (`009_servo_plate`) y pernos metálicos M3 en cada articulación.
- **Cinemática Analítica de Lazo Cerrado (0.000 mm de error)**: Resolución trigonométrica exacta en cada cuadro mediante intersección analítica de circunferencias (`intersectCircles`) para los **3 paralelogramos mecánicos acoplados**:
  1. **Lazo #1**: Brazo principal (`004`) + biela trasera pasiva (`003`) + escuadra triangular (`006`).
  2. **Lazo #2**: Palanca vertical de codo (`002`) + biela trasera activa (`003`) + antebrazo horizontal (`005`).
  3. **Lazo #3**: Biela superior delantera (`003`) + soporte de muñeca/pinza (`012`), manteniendo la pinza siempre paralela al suelo.
- **Telemetría en Vivo y Objeto Interactivo**:
  - Indicador superpuesto con los ángulos actuales de los 4 ejes y las **coordenadas espaciales `XYZ` en milímetros** de la punta de la pinza.
  - Objeto de prueba sobre pedestal con detección física de agarre y gravedad simulada para ensayar rutinas de *Pick & Place* incluso sin tener la placa conectada (**Modo Simulador**).
- **Cámara Orbital y Personalización Visual**:
  - Rotación orbital 3D con clic izquierdo, zoom con rueda del ratón y **4 vistas rápidas de cámara** (*Vista 3/4*, *Lateral*, *Frente* y *Superior*).
  - Selector de **Color PLA** en vivo para igualar el color del filamento real del brazo.
  - Conmutador **☀️ Modo Claro / 🌙 Modo Oscuro**.

### 🎛️ 2. Control de Ejes y Diseño UX/UI (Leyes de UX + Principios de Figma)
- **Layout de Altura Completa (100% Viewport)**: Visor 3D a la izquierda y centro de control a la derecha alineados de extremo a extremo sin necesidad de hacer scroll en la página.
- **Control Individual de los 4 Ejes con Identidad Semántica**:
  - **1. Base (`D8`)**: Giro horizontal con presets `◄ Izquierda | Centro | Derecha ►`.
  - **2. Hombro (`D9`)**: Avance/retroceso con presets `◄ Atrás | Vertical | Adelante ►`.
  - **3. Codo (`D10`)**: Elevación con presets `▼ Abajo | Medio | ▲ Arriba`.
  - **4. Pinza (`D11`)**: Apertura/cierre con presets `🔓 Abrir | Media | 🔒 Cerrar`.
- **Retroalimentación Visual Inmediata (Umbral de Doherty y Ley de Fitts)**:
  - Resaltado automático del botón de preset activo cuando el eje alcanza su posición mínima, central (`90°`) o máxima.
  - Botón individual `Soltar` por eje y parada de emergencia general **`🛑 Soltar Servos (X)`**.
- **Revelación Progresiva (`⚙️ Límites` — Ley de Hick)**:
  - Cajón desplegable bajo demanda para calibrar los límites físicos `Mín °` y `Máx °` de cada servo sin sobrecargar visualmente la interfaz durante el uso diario.

### 🎬 3. Centro Multifunción en 3 Pestañas (Ley de Miller)
1. **🎬 Secuencias (Grabador y Reproductor *Pick & Place*)**:
   - Grabación de poses paso a paso (`➕ Guardar Pose`), reproducción continua en bucle (`Loop`) o pasada única, resaltado en verde del paso activo con auto-scroll y carga de secuencia demostrativa de 9 pasos (`✨ Cargar Demo`).
   - Exportación de secuencias a archivo `.json` (`💾 Exportar`).
2. **🎮 Poses Rápidas y Control por Teclado**:
   - **6 Posturas Globales en 1 Clic**: *🏠 Reposo Central*, *📦 Ir al Objeto*, *🦅 Alcance Frontal*, *🏗️ Torre Elevada*, *📤 Zona Descarga* y animación de *👋 Saludo Rápido*.
   - **Modo Teclado en Vivo (`⌨️ Activar Teclas`)**: Manejo directo tipo videojuego con iluminación de teclas en pantalla:
     - `A` / `D`: Giro de Base
     - `W` / `S`: Hombro Adelante / Atrás
     - `↑` / `↓`: Codo Arriba / Abajo
     - `Espacio`: Alternar Pinza Abierta / Cerrada
     - `H`: Volver a Pose Inicial (`90°`)
3. **📟 Consola Serie Bidireccional**:
   - Monitor en tiempo real de las tramas JSON enviadas por el Arduino Nano (`115200` baudios), indicador de nuevos eventos, botón de inspección de memoria **`📊 Leer EEPROM`** y entrada manual de comandos.

---

## ⚡ Características del Firmware Arduino Nano (`brazo_robot.ino`)

- **Arranque Seguro sin Brusquedades (`Safe Startup`)**: Al encender o reiniciar el Arduino Nano, los 4 servos permanecen desenergizados (`detach()`) hasta recibir el primer comando, evitando el clásico sacudón inicial al energizar la fuente.
- **Interpolación de Movimiento Suavizada No Bloqueante**: Bucle temporal con `millis()` que desplaza cada servo grado a grado según la velocidad configurada (`stepDelayMs`, por defecto `15 ms/°`), logrando trayectorias fluidas y cuidando los engranajes de los servos SG90.
- **Persistencia Inteligente en Hardware (`EEPROM.update()`)**:
  - Guarda los límites `Mín/Máx` de los 4 servos y la velocidad `stepDelayMs` en la memoria **EEPROM interna del ATmega328P** (con byte mágico de validación `0xEA`).
  - Utiliza `EEPROM.update()` en lugar de `EEPROM.write()`: sólo escribe en las celdas cuyos bytes realmente cambiaron y únicamente al modificar la configuración (nunca durante el movimiento de los servos), protegiendo la vida útil de los 100.000 ciclos de la EEPROM.
- **Restauración a Valores de Fábrica (`RESET`)**: Comando dedicado que restablece en un clic tanto el `localStorage` del navegador como la `EEPROM` del Arduino Nano.

### 📡 Protocolo de Comunicación Serie (`115200` baudios)

| Comando | Ejemplo | Descripción |
| :--- | :--- | :--- |
| `S<0..3>:<ángulo>` | `S0:120` | Mueve el servo indicado (`0`=Base, `1`=Hombro, `2`=Codo, `3`=Pinza) al ángulo objetivo respetando sus límites. |
| `A:<s0>,<s1>,<s2>,<s3>` | `A:90,90,90,90` | Mueve los 4 servos simultáneamente hacia una pose completa. |
| `V:<ms>` | `V:15` | Ajusta el retardo en milisegundos por grado (`0` = instantáneo, `15` = suave) y lo guarda en EEPROM. |
| `L<0..3>:<min>,<max>` | `L2:15,165` | Actualiza los límites de seguridad mínimo y máximo del servo y los persiste en EEPROM. |
| `D<0..3>` | `D1` | Desenergiza (`detach`) únicamente el servo indicado. |
| `X` / `STOP` | `X` | **Parada de emergencia**: desenergiza inmediatamente los 4 servos. |
| `RESET` / `FACTORY` | `RESET` | Restaura los límites y velocidad de fábrica en la memoria EEPROM. |
| `STATUS` / `?` | `STATUS` | Devuelve una trama JSON con ángulos actuales, objetivos, estado de conexión, límites y velocidad. |

---

## 🛠️ Backend y Arquitectura Tecnológica

El proyecto fue diseñado bajo una arquitectura **Zero-Dependency Local Stack** (sin servidores intermedios ni necesidad de conexión a Internet):

1. **Motor Gráfico 3D (`three.min.js` — Three.js r128)**:
   - Iluminación física (`MeshStandardMaterial` y `MeshPhysicalMaterial`) con sombras suaves `PCFSoftShadowMap`, luces direccionales y luz de recorte (*rim light*).
2. **Empaquetado Binario de Geometrías CAD (`eezybotarm_stl.js`)**:
   - Las 13 mallas `.STL` originales fueron procesadas para centrar sus orígenes locales `(0, 0, 0)` exactamente en los ejes de los pernos de `Ø4 mm`.
   - Los vértices fueron cuantizados en arreglos tipados `Int16Array` (resolución de `0.02 mm`), las normales en `Int8Array` y los índices topológicos en `Uint16Array`, codificados en Base64 para cargarse instantáneamente desde `file:///` sin restricciones de CORS.
3. **Comunicación Directa Hardware-Navegador (`Web Serial API`)**:
   - Conexión serie nativa mediante `navigator.serial` a `115200` baudios usando `TextEncoderStream` y `TextDecoderStream`, con manejo limpio de cancelación de *streams* para reconectar sin bloquear el puerto COM.
4. **Persistencia Dual Sincronizada (`localStorage` + `EEPROM`)**:
   - Cualquier cambio en límites o velocidad se almacena en el navegador (`localStorage`) y se transmite por puerto serie al Arduino Nano (`EEPROM`), manteniendo ambos entornos sincronizados al reconectar.

---

## 🔌 Conexión en Arduino Nano + Sensor Shield

| Eje del Brazo | Pin Digital (Shield) | Cable Marrón / Negro | Cable Rojo | Cable Naranja / Amarillo |
| :--- | :---: | :---: | :---: | :---: |
| **1. Base (Giro Horizontal)** | **D8** | `G` (GND) | `V` (+5V) | `S` (Señal) |
| **2. Hombro (Adelante / Atrás)** | **D9** | `G` (GND) | `V` (+5V) | `S` (Señal) |
| **3. Codo (Arriba / Abajo)** | **D10** | `G` (GND) | `V` (+5V) | `S` (Señal) |
| **4. Pinza (Gripper)** | **D11** | `G` (GND) | `V` (+5V) | `S` (Señal) |

### 🚀 Compilación y Carga del Firmware (`brazo_robot.ino`)

Podés cargarlo desde el **Arduino IDE** (seleccionando *Placa: Arduino Nano* y *Procesador: ATmega328P (Old Bootloader)*) o directamente por línea de comandos con **Arduino CLI**:

```powershell
arduino-cli compile --fqbn arduino:avr:nano:cpu=atmega328old brazo_robot.ino
arduino-cli upload -p COM3 --fqbn arduino:avr:nano:cpu=atmega328old brazo_robot.ino
```

---

## 📄 Licencia

Este proyecto se distribuye bajo la **Licencia MIT**. Consultá el archivo [LICENSE](LICENSE) para más detalles.
