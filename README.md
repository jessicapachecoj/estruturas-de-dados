# 📦 Algoritmo de Huffman Adaptativo (FGK)

**Foco:** Implementação do algoritmo de **Compressão sem Perdas de Huffman Adaptativo (variante FGK)** em C. Demonstra domínio sobre **árvores binárias dinâmicas, manipulação de bits** e o desafio de **sincronização de estado em codificação de passagem única**.

---

## 1. 📖 Contexto e Propósito do Projeto

Este projeto aborda o desafio de **compressão de dados onde as frequências de símbolos não são conhecidas previamente**. O algoritmo **FGK (Faller-Gallager-Knuth)** atualiza a árvore de codificação/decodificação dinamicamente à medida que cada símbolo é processado.

O trabalho comprova o domínio sobre:

- **Estruturas de Dados Dinâmicas:** Uso de ponteiros e alocação de memória para construir e modificar a árvore de Huffman em tempo real.  
- **Otimização:** Lógica de codificação sem perdas e efeito da variação de frequência no tamanho final do arquivo.  
- **Manipulação de Baixo Nível:** Leitura/escrita de bits para garantir a menor representação possível do código.

---

## 2. 🏗️ Arquitetura e Funcionamento (FGK)

A arquitetura se baseia em uma **estrutura de nó (struct Node)** que, além do peso e do símbolo, contém um campo `order` para manter a **propriedade crucial de balanceamento**.

### 🧠 Regra de Atualização (Update Rule)

O coração do algoritmo é a função `update_tree`, que garante que a árvore permaneça em um estado válido para codificador e decodificador:

- **Expansão do NYT:** Se um símbolo é novo, o nó **NYT (Not Yet Transmitted)** é expandido, criando uma folha para o novo símbolo e um novo nó NYT (peso 0).  
- **Propriedade Sibling:** Em cada incremento de peso, o algoritmo verifica se o nó precisa ser trocado (`swap_nodes`) com o nó líder do bloco de peso (maior `order` e mesmo `weight`), mantendo a árvore ordenada por peso de forma ascendente.  
- **Sincronia:** Codificador e decodificador devem aplicar exatamente a **mesma sequência de atualizações** para que a decodificação seja perfeita (bit a bit).

### ⚙️ Componentes de I/O de Bits

- **write_bit / read_bit:** Funções críticas para o controle exato do fluxo binário. Leem/escrevem um bit por vez em um buffer de 8 bits, essencial para a compressão (Huffman não usa bytes completos).  
- **Tratamento NYT:** Novos símbolos são codificados com o **código do NYT**, seguido por uma representação literal de **9 bits** para acomodar o alfabeto estendido (0-256, incluindo o `EOT_SYMBOL`).

---

## 3. 🧪 Testes e Desafios Técnicos

O projeto foi um exercício de rigor algorítmico, expondo a extrema sensibilidade da compressão.

### Desafios Encontrados

- **Rigor Bit a Bit:** Falhas nos testes de interoperabilidade mostraram que pequenos desvios na atualização ou no I/O (especialmente o flush do último byte) quebram a sincronia com implementações externas.  
- **Robustez Binária:** Testes com arquivos binários (`cachorro.webp`) confirmaram que o código manipula qualquer fluxo de 8 bits sem corrupção.  
- **Taxa de Compressão:** Arquivos já comprimidos (.webp) geram **compressão negativa**, pois o overhead do algoritmo supera o ganho de codificação.

---

## 4. 🛠️ Tecnologias e Ferramentas

- **Linguagem:** C (controle de memória e manipulação de bits de baixo nível).  
- **Estruturas:** Árvores Binárias Dinâmicas, Arrays de Mapeamento (para otimizar acesso).  
- **Algoritmos:** Algoritmo de Huffman Adaptativo FGK, Regra de **Sibling Property**.
