# EDAversi

## Integrantes del grupo y contribución al trabajo de cada integrante

* [Nombre 1]: [Describir la parte en la que participó: por ejemplo, lógica del motor de IA, interfaz gráfica, pruebas, documentación, etc.]
* [Nombre 2]: [Contribución]
* [Nombre 3]: [Contribución]

[Agregar más integrantes o detalles si corresponde]

---

## Parte 1: Generación de movimientos válidos y algoritmo de jugada

Se implementó la función para generar todos los movimientos válidos en cada turno de acuerdo a las reglas del juego Reversi.  
**Pruebas realizadas:**
- Se probaron posiciones estándar del inicio, mitad y final del juego para verificar que la cantidad y los lugares de los movimientos válidos fueran correctos.
- Se validaron escenarios límite (bordes, esquinas, situaciones sin movimientos posibles).
- Se comprobó contra partidas reales y manuales, asegurando que la IA sólo ofreciera y permitiera movimientos legítimos.
- Se automatizaron algunas pruebas con tableros preconfigurados para detectar errores en la captura de fichas en todas las direcciones.

---

## Parte 2: Implementación del motor de IA

El motor de IA se basa en el algoritmo **Minimax** con varias optimizaciones:
- Se simula el árbol de posibles jugadas alternando entre la IA y el oponente.
- Se implementó poda alfa-beta para reducir la cantidad de nodos explorados, descartando ramas que no influyen en la decisión final.
- Se utiliza una función de evaluación que pondera:
  - Diferencia de fichas.
  - Control de esquinas y bordes.
  - Movilidad (cantidad de movimientos posibles para cada jugador).
  - Uso de matrices de pesos para valorar cada casilla del tablero según su importancia estratégica.
- Se limita la profundidad del árbol y la cantidad de nodos explorados según la fase del juego para asegurar respuestas rápidas de la IA.
- Se probaron distintos pesos y heurísticas para optimizar el rendimiento y la calidad de las jugadas.

---

## Parte 3: Poda del árbol

El algoritmo minimax básico explora todas las posibles combinaciones de jugadas hasta una cierta profundidad.  
**Problema:**  
La complejidad crece exponencialmente:  
- Si hay en promedio 10 movimientos posibles por turno y se mira a 7 turnos adelante, hay del orden de 10^7 nodos.
- En el final del juego, la profundidad puede aumentar mucho, haciendo inviable recorrer todo el árbol.

**Soluciones aplicadas para controlar la complejidad:**
- **Poda alfa-beta:** Permite ignorar ramas que no aportan mejores resultados, reduciendo la cantidad de nodos explorados en hasta 10-100x en la práctica.
- **Límite de profundidad adaptativo:** Ajusta la cantidad de turnos simulados según la fase del juego. Menos profundidad al principio, más al final.
- **Límite de nodos:** Se interrumpe la búsqueda si se superan los 100.000 nodos, garantizando que el programa nunca se “congele”.

**Justificación:**  
Sin estas podas y límites, el algoritmo sería impracticable para jugar en tiempo real debido a la explosión combinatoria.

---

## Documentación adicional


## Bonus points
