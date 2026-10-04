# Sistemas Operativos 2026/27 — Trabalho Prático 1
## Simulador de Escalonamento

### 1. Identificação

**Autores:** [PREENCHER NOME(S)]  
**Repositório GitHub:** [PREENCHER LINK]

---

## 2. Implementação

O simulador foi estendido para suportar os quatro algoritmos pedidos:

- **FIFO** — algoritmo fornecido, não preemptivo.
- **SJF (Shortest Job First)** — escolhe, entre os processos na READY queue, o burst de CPU com menor duração. É não preemptivo.
- **RR (Round-Robin)** — utiliza um *time-slice* de **500 ms**. Quando o processo não terminou o burst no fim do *time-slice*, é retirado do CPU e colocado no fim da READY queue.
- **MLFQ (Multi-Level Feedback Queue)** — utiliza três níveis de prioridade, quantum de 500 ms por fila e 1000 ms de alocação acumulada por nível, com *priority boost* periódico de 5000 ms que move todos os processos do sistema (CPU, READY, BLOCKED e COMMAND) para o nível 0. Um processo desce de nível quando esgota a sua alocação acumulada no nível, mesmo que tenha cedido o CPU voluntariamente entre bursts. Novos processos entram no nível 0. Quando existem processos de prioridade diferente, o de maior prioridade executa; processos com a mesma prioridade usam round-robin.

Foram também acrescentadas mensagens de diagnóstico ao escalonador para evidenciar:

- despacho de um processo para o CPU;
- primeira utilização do CPU e respetivo tempo de resposta;
- preempção no RR;
- preempção e alteração de nível no MLFQ;
- *priority boost* do MLFQ.

---

## 3. Cenários utilizados

Foram criados cenários específicos para evidenciar os comportamentos pedidos.

### 3.1 SJF

O cenário `scenarios/trabalho/sjf/` contém cinco processos:

| Processo | CPU (ms) |
|---|---:|
| A | 3000 |
| B | 300 |
| C | 300 |
| D | 300 |
| E | 300 |

A intenção é colocar um processo longo juntamente com quatro processos curtos. No SJF, os processos curtos são escolhidos antes do processo longo, reduzindo o tempo médio de conclusão.

### 3.2 Resposta / Round-Robin

O cenário `scenarios/trabalho/rr/` contém novamente cinco processos, com um processo longo de 4000 ms e quatro processos curtos de 300 ms. O cenário permite observar a diferença entre escalonamento não preemptivo e preemptivo através do *time-slice* de 500 ms.

> **Nota sobre o enunciado:** o ponto 3.b da versão fornecida apresenta uma inconsistência ao mencionar FIFO como algoritmo comparado consigo próprio. Por isso, o cenário foi preparado para evidenciar o comportamento de resposta do escalonamento preemptivo RR face aos algoritmos não preemptivos.

### 3.3 MLFQ — dois interativos e um batch

Em `scenarios/trabalho/mlfq1/` existem:

- A — interativo, com bursts de CPU curtos e bloqueios de E/S;
- B — interativo, com bursts de CPU curtos e bloqueios de E/S;
- C — batch, com um burst longo de CPU.

O objetivo é evidenciar que A e B permanecem em níveis de prioridade elevados enquanto C consome *time-slices* completos e desce de nível.

### 3.4 MLFQ — batch → interativo → batch

Em `scenarios/trabalho/mlfq2/`, o processo B apresenta a sequência:

1. burst longo de CPU — comportamento batch;
2. vários bursts curtos com bloqueio — comportamento interativo;
3. novo burst longo de CPU — volta ao comportamento batch.

Os processos A e C são interativos. Este cenário permite observar a alteração de comportamento do processo B ao longo da execução.

---

## 4. Resultados

Os tempos seguintes foram obtidos através de execuções do simulador. Os tempos de `Elapsed` são apresentados em segundos e foram calculados a partir da saída das aplicações. O tempo de resposta corresponde ao intervalo entre a entrada inicial na READY queue e a primeira atribuição do CPU.

### 4.1 Cenário SJF/RR — tempo de execução

Média das execuções utilizadas:

| Aplicação | FIFO (s) | SJF (s) | RR (s) | MLFQ (s) |
|---|---:|---:|---:|---:|
| A | 3.231 | 4.240 | 4.242 | 4.238 |
| B | 2.624 | 0.804 | 0.741 | 1.043 |
| C | 3.733 | 0.602 | 1.044 | 1.413 |
| D | 3.931 | 0.808 | 1.612 | 1.245 |
| E | 2.821 | 0.801 | 1.145 | 0.841 |
| **Média das aplicações** | **3.268** | **1.451** | **1.757** | **1.756** |

A execução evidencia o efeito do SJF: os processos curtos são executados antes do processo longo. No RR e no MLFQ, o processo longo é interrompido por *time-slices*, permitindo que os restantes processos obtenham CPU antes da sua conclusão.

### 4.2 Cenário SJF/RR — tempo de resposta médio

| Algoritmo | Tempo médio de resposta (s) |
|---|---:|
| FIFO | 2.420 |
| SJF | 0.603 |
| RR | 0.726 |
| MLFQ | 0.727 |

Estes valores correspondem ao primeiro despacho para CPU de cada processo nas execuções utilizadas. A ordem efetiva dos processos pode variar ligeiramente devido ao momento em que as aplicações estabelecem as ligações ao simulador.

### 4.3 MLFQ — cenário com dois interativos e um batch

| Aplicação | CPU (s) | BLOCKED (s) | Elapsed médio (s) |
|---|---:|---:|---:|
| A | 0.800 | 2.000 | 3.836 |
| B | 1.000 | 2.400 | 3.736 |
| C | 2.500 | 0.000 | 4.342 |

O processo C utiliza bursts longos e sofre preempções sucessivas, descendo para níveis inferiores. A e B terminam os seus bursts rapidamente e passam para bloqueio de E/S, permanecendo em níveis elevados.

Tempo de resposta inicial observado: A = 0.252 s, B = 0.000 s, C = 0.454 s.

### 4.4 MLFQ — cenário batch → interativo → batch

| Aplicação | CPU (s) | BLOCKED (s) | Elapsed médio (s) |
|---|---:|---:|---:|
| A | 0.800 | 2.000 | 3.585 |
| B | 5.900 | 2.100 | 9.029 |
| C | 1.000 | 2.400 | 3.938 |

Neste cenário, B começa com um burst longo e é tratado como processo CPU-bound. Durante a fase de bursts curtos com E/S, passa a apresentar comportamento semelhante ao de um processo interativo. No final, volta a executar bursts longos e volta a consumir *time-slices* completos.

Tempo de resposta inicial observado: A = 0.000 s, B = 0.454 s, C = 0.202 s.

---

## 5. Análise dos algoritmos

### FIFO

O FIFO mantém a ordem de chegada à READY queue. Não existe preempção: depois de atribuído o CPU, o processo executa até terminar o burst. Esta característica torna o comportamento simples, mas um processo com um burst longo pode atrasar processos que necessitam de pouco CPU.

### SJF

O SJF compara os tempos dos bursts presentes na READY queue e escolhe o menor. No cenário criado, os quatro processos de 300 ms são escolhidos antes do processo de 3000 ms. Consequentemente, os processos curtos terminam mais cedo e o tempo médio de conclusão observado diminui.

### RR

O RR divide a utilização do CPU em intervalos de 500 ms. Quando o processo não termina dentro desse intervalo, é preemptado e colocado novamente no fim da READY queue. O resultado é uma alternância entre processos, permitindo que vários processos obtenham CPU antes de um processo longo terminar.

### MLFQ

O MLFQ acrescenta prioridades à ideia do RR. Dentro de cada nível, processos com a mesma prioridade usam round-robin. O tempo de CPU consumido num nível é acumulado entre bursts; ao atingir a alocação definida para esse nível, o processo desce de nível mesmo que tenha cedido o CPU voluntariamente. O *priority boost* periódico devolve todos os processos ao nível superior, incluindo os que estejam em READY, BLOCKED, COMMAND ou no CPU, evitando que um processo permaneça indefinidamente num nível baixo.

Os cenários com E/S mostram a diferença entre processos interativos e batch: os processos interativos abandonam o CPU rapidamente para esperar por E/S, enquanto os processos batch tendem a utilizar *time-slices* completos.

---

## 6. Conclusão

A implementação permite selecionar o algoritmo através do argumento `--sched`:

```text
./build/ossim --sched FIFO
./build/ossim --sched SJF
./build/ossim --sched RR
./build/ossim --sched MLFQ
```

Foram acrescentados mecanismos de seleção SJF, preempção RR, níveis de prioridade MLFQ e *priority boost*, bem como mensagens de diagnóstico para demonstrar as decisões do escalonador durante a execução.
