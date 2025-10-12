# EDAversi

## Integrantes del grupo y contribución al trabajo de cada integrante

* [Brusasca Juan Luis]: Implementación de la ia, mejoramiento de la eficiencia del programa, tester del algoritmo.
* [Chiusaroli Francisco]: Implementación de la ia, mejoramiento de la eficiencia del programa, tester del algoritmo.
* [Forchiassin Luca]: Implementación de la ia, mejoramiento de la eficiencia del programa, tester del algoritmo.
* [Garcilazo Tomás]: Implementación de la ia, mejoramiento de la eficiencia del programa, tester del algoritmo.

## Parte 1: Generación de movimientos válidos y algoritmo de jugada

Se implementó la función getValidMoves() que genera todos los movimientos válidos en cada turno. Esta:

- Recorre todas las casillas vacías del tablero
- Para cada casilla vacía, verifica en las 8 direcciones posibles
- Comprueba si hay fichas del oponente adyacentes seguidas de al menos una ficha propia
- Solo agrega movimientos que cumplan con la regla de captura del Reversi

Para verificar que funcione bien, se validaron escenarios límite (bordes, esquinas, situaciones sin movimientos posibles) y 
se comprobó contra partidas reales y manuales, asegurando que la IA sólo ofreciera y permitiera movimientos legítimos.

## Parte 2: Implementación del motor de IA

El motor de IA se implementó utilizando el algoritmo Minimax y una función de evaluación basada en el trabajo "Estrategia Reversista" de Lea Tosti.
Características principales:
Función de evaluación multi-criterio, es decir, la evaluación se adapta según la fase del juego (early/mid/end game) y pondera:

- Movilidad: Cantidad de movimientos válidos disponibles
- Movilidad potencial: Casillas vacías adyacentes a fichas del oponente
- Control de esquinas: Las 4 esquinas son las posiciones más valiosas
- Estabilidad: Fichas que no pueden ser capturadas
- Estabilidad de bordes: Control de las filas y columnas externas
- Conteo de fichas: Con estrategia variable según fase:

  - Early game: Negativo (-2 a -3) - tener menos fichas es mejor
  - Mid game: Neutral (0) o ligeramente negativo
  - End game: Positivo (10-15) - maximizar fichas propias


- Frontera: Penalización por fichas expuestas con casillas vacías adyacentes
- Evaluación posicional: Matriz de pesos estratégicos para cada casilla del tablero

Matriz de pesos posicionales:
   A    B    C   D   E   F    G    H
1: 120 -20  20   5   5  20  -20  120
2: -20 -40  -5  -5  -5  -5  -40  -20
3:  20  -5  15   3   3  15   -5   20
4:   5  -5   3   3   3   3   -5    5
5:   5  -5   3   3   3   3   -5    5
6:  20  -5  15   3   3  15   -5   20
7: -20 -40  -5  -5  -5  -5  -40  -20
8: 120 -20  20   5   5  20  -20  120

También se implementaron algunas optimizaciones:

- Poda alfa-beta: Elimina ramas que no pueden mejorar la decisión final
- Ordenamiento de movimientos: Evalúa primero las esquinas y posiciones más prometedoras para mejorar la eficiencia de la poda
- Límite de nodos adaptable
- Búsqueda iterativa: Se profundiza gradualmente hasta alcanzar el límite de nodos

## Parte 3: Poda del árbol

El algoritmo minimax básico tiene un problema de complejidad:
Sin poda: O(b^d) donde:

b = factor de ramificación
d = profundidad del árbol

Esto lo hace impráctico para grandes profundidades.

Para ello se implementó: 

1. Poda alfa-beta, la cual mantiene dos valores durante la búsqueda:

- Alfa: Mejor valor garantizado para el jugador maximizador
- Beta: Mejor valor garantizado para el jugador minimizador

Si en algún momento β ≤ α, se puede podar la rama completa porque no influirá en la decisión final.

2. Ordenamiento de movimientos heurístico
Los movimientos se ordenan antes de explorarlos para maximizar la eficiencia de la poda:

- Esquinas (mayor prioridad)
- Bordes estables
- Posiciones con mayor valor posicional
- Resto de movimientos

Esto permite encontrar movimientos óptimos temprano y podar más ramas.

3. Límite de profundidad adaptativo
Se ajusta según la fase del juego:

- Early game: Menor profundidad (menos opciones estratégicas críticas)
- End game: Mayor profundidad (decisiones más determinantes)

4. Detección de fin de juego temprano
Si no hay movimientos válidos para ningún jugador, se evalúa inmediatamente sin continuar la búsqueda.

## Documentación adicional

- "Estrategia Reversista", Lea Tosti para la estrategia de la ia
- Claude AI, para el mejoramiento de la eficiencia del programa

## Bonus points

- Implementación circulos que te indican los posibles movimientos
- implemenación de un circulo amarillo que indica el último movimiento 
