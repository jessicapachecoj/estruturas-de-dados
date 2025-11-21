#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Constantes Globais ---
// Define o marcador de Fim de Transmissão.
#define EOT_SYMBOL 256
// Define o número total de símbolos (0-255 ASCII + EOT).
#define SYMBOL_COUNT (EOT_SYMBOL + 1)
// Define o número máximo de nós na árvore, necessário para o array `nodes`.
#define NODE_COUNT (SYMBOL_COUNT * 2 - 1)
// Identificador para nós que não são folhas.
#define INTERNAL_NODE -1

// --- Estrutura de Dados para o Nó da Árvore ---
typedef struct Node {
    struct Node *parent, *left, *right; // Ponteiros para a estrutura da árvore.
    int weight;   // Frequência do nó, usado para o balanceamento.
    int symbol;   // Valor do símbolo (0-256) se for um nó folha.
    int order;    // Número de ordem único para manter a "sibling property".
} Node;

// --- Variáveis Globais ---
Node* root = NULL;                   // Ponteiro para a raiz da árvore.
Node* leaves[SYMBOL_COUNT] = {NULL}; // Array de acesso rápido para as folhas.
Node* nodes[NODE_COUNT + 1] = {NULL}; // Array para encontrar nós pela sua ordem.
Node* NYT_NODE = NULL;               // Ponteiro para o nó "Not Yet Transmitted".
int next_node_order;                 // Contador para atribuir a ordem aos nós.

unsigned char bit_buffer = 0;        // Buffer para escrita/leitura de bits.
int bit_count = 0;                   // Contador de bits no buffer.

// --- Protótipos das Funções ---
void initialize_tree();
void encode_symbol(FILE* out, int symbol);
void update_tree(int symbol);
void compressFile(const char* input_path, const char* output_path);
void decompressFile(const char* input_path, const char* output_path);

// Ponto de entrada do programa. Processa os argumentos da linha de comando.
int main(int argc, char* argv[]) {
    // Valida os argumentos de entrada.
    if (argc != 4) {
        fprintf(stderr, "Uso:\n  Compactar:    %s -c <arquivo_entrada> <arquivo_saida>\n", argv[0]);
        fprintf(stderr, "  Descompactar: %s -d <arquivo_entrada> <arquivo_saida>\n", argv[0]);
        return 1;
    }
    // Direciona para a função apropriada.
    if (strcmp(argv[1], "-c") == 0) compressFile(argv[2], argv[3]);
    else if (strcmp(argv[1], "-d") == 0) decompressFile(argv[2], argv[3]);
    else fprintf(stderr, "Opcao invalida.\n");
    return 0;
}

// --- Funções de Gerenciamento da Árvore ---

// Aloca e inicializa um novo nó.
Node* create_node(int symbol, int weight, Node* parent) {
    Node* p = (Node*)malloc(sizeof(Node));
    if(p == NULL) exit(1);
    p->symbol = symbol;
    p->weight = weight;
    p->parent = parent;
    p->left = NULL;
    p->right = NULL;
    p->order = next_node_order--; // Atribui ordem decrescente.
    nodes[p->order] = p;
    return p;
}

// Libera recursivamente toda a memória alocada para a árvore.
void free_tree(Node* p) {
    if (p == NULL) return;
    free_tree(p->left);
    free_tree(p->right);
    // Limpa a referência no array de nós antes de liberar.
    if (p->order > 0) nodes[p->order] = NULL;
    free(p);
}

// Reseta o estado da árvore e das variáveis globais para uma nova operação.
void initialize_tree() {
    // Limpa a árvore anterior, se existir, para evitar vazamento de memória.
    if (root != NULL) {
        for(int i = 1; i <= NODE_COUNT; i++) {
            if (nodes[i]) {
                free(nodes[i]);
                nodes[i] = NULL;
            }
        }
    }
    for(int i=0; i < SYMBOL_COUNT; ++i) leaves[i] = NULL;
    
    // Reseta o estado inicial.
    bit_buffer = 0;
    bit_count = 0;
    next_node_order = NODE_COUNT;
    root = NYT_NODE = create_node(INTERNAL_NODE, 0, NULL);
}

// Troca a posição e os metadados de dois nós na árvore.
void swap_nodes(Node* p, Node* q) {
    // Troca as posições nos arrays de atalho para as folhas.
    if (p->symbol != INTERNAL_NODE) leaves[p->symbol] = q;
    if (q->symbol != INTERNAL_NODE) leaves[q->symbol] = p;

    // Troca os ponteiros dos pais para apontarem para os nós trocados.
    Node* p_parent = p->parent;
    Node* q_parent = q->parent;
    if (p_parent->left == p) p_parent->left = q; else p_parent->right = q;
    if (q_parent->left == q) q_parent->left = p; else q_parent->right = p;
    
    // Troca os ponteiros de pai dos próprios nós.
    p->parent = q_parent;
    q->parent = p_parent;

    // Troca as ordens, que é crucial para o critério de desempate.
    int temp_order = p->order;
    p->order = q->order;
    q->order = temp_order;

    // Atualiza o array de acesso por ordem com os nós nas novas posições.
    nodes[p->order] = p;
    nodes[q->order] = q;
}

// Atualiza a árvore de Huffman após processar um símbolo.
// Esta é a função central do algoritmo adaptativo.
void update_tree(int symbol) {
    Node* p = leaves[symbol];

    if (p == NULL) { // Caso 1: Símbolo novo.
        // Expande o nó NYT, criando um nó interno com dois filhos:
        // a folha para o novo símbolo e um novo nó NYT.
        Node* old_nyt = NYT_NODE;
        old_nyt->symbol = INTERNAL_NODE;
        old_nyt->right = create_node(symbol, 1, old_nyt);
        old_nyt->left = create_node(INTERNAL_NODE, 0, old_nyt);
        
        // Atualiza os ponteiros de atalho.
        leaves[symbol] = old_nyt->right;
        NYT_NODE = old_nyt->left;
        
        p = old_nyt; // O nó que começa a ser atualizado é o pai recém-modificado.
    }

    // Sobe da folha até a raiz, rebalanceando a árvore para manter a "sibling property".
    while (p != NULL) {
        Node* leader = p;
        // Encontra o nó de maior ordem (líder) no mesmo bloco de peso.
        // A busca linear é ineficiente, mas garante a correção lógica.
        for (int i = 1; i <= NODE_COUNT; i++) {
            if (nodes[i] && nodes[i]->weight == p->weight) {
                if (nodes[i]->order > leader->order) {
                    leader = nodes[i];
                }
            }
        }
        
        // Se o nó atual não for o líder e não for seu ancestral direto, troca-os.
        if (leader != p && p->parent != leader) {
            swap_nodes(p, leader);
        }

        // Incrementa o peso e continua subindo para o pai.
        p->weight++;
        p = p->parent;
    }
}

// --- Funções de I/O de Bits e Compressão/Descompressão ---

// Obtém o código binário de um nó, subindo na árvore até a raiz.
void get_path(Node* p, char* path, int* len) {
    *len = 0;
    if (p->parent == NULL) return; // Raiz não tem código.

    Node* current = p;
    // O caminho é construído de baixo para cima.
    do {
        if (current->parent->left == current) path[(*len)++] = '0';
        else path[(*len)++] = '1';
        current = current->parent;
    } while (current->parent != NULL);

    // Inverte o caminho para a ordem correta (raiz -> folha).
    for (int i = 0, j = *len - 1; i < j; ++i, --j) {
        char temp = path[i]; path[i] = path[j]; path[j] = temp;
    }
}

// Acumula bits em um buffer e o escreve no arquivo quando atinge 8 bits.
void write_bit(FILE* file, int bit) {
    bit_buffer = (bit_buffer << 1) | bit;
    if (++bit_count == 8) {
        fputc(bit_buffer, file);
        bit_count = 0;
        bit_buffer = 0;
    }
}

// Escreve os bits restantes no buffer no final da compressão.
void flush_bits(FILE* file) {
    while (bit_count > 0 && bit_count < 8) {
        bit_buffer <<= 1;
        bit_count++;
    }
    if (bit_count == 8) {
        fputc(bit_buffer, file);
    }
}

// Lê um byte do arquivo e fornece um bit de cada vez, do mais significativo ao menos.
int read_bit(FILE* file) {
    if (bit_count == 0) {
        int c = fgetc(file);
        if (c == EOF) return EOF;
        bit_buffer = (unsigned char)c;
        bit_count = 8;
    }
    // Pega o bit mais à esquerda e depois o descarta na próxima chamada.
    return (bit_buffer >> --bit_count) & 1;
}

// Escreve o código de um nó no arquivo.
void write_code(FILE* file, Node* node) {
    char buffer[64]; 
    int len;
    get_path(node, buffer, &len);
    for (int i = 0; i < len; ++i) {
        write_bit(file, buffer[i] - '0');
    }
}

// Orquestra a codificação de um único símbolo.
void encode_symbol(FILE* out, int symbol) {
    // Caso 1: Símbolo já existe na árvore. Envia seu código.
    if (leaves[symbol]) {
        write_code(out, leaves[symbol]);
    } else { // Caso 2: Símbolo novo. Envia o código do NYT, seguido do valor literal.
        write_code(out, NYT_NODE);
        // Usa 9 bits para um símbolo novo para poder representar o EOT (256).
        for (int i = 8; i >= 0; --i) {
            write_bit(out, (symbol >> i) & 1);
        }
    }
    // Atualiza a árvore após cada símbolo para manter a sincronia.
    update_tree(symbol);
}

// Função principal para compressão de um arquivo.
void compressFile(const char* input_path, const char* output_path) {
    FILE* input = fopen(input_path, "rb");
    FILE* output = fopen(output_path, "wb");
    if (!input || !output) { perror("Erro ao abrir arquivos"); return; }
    
    initialize_tree();
    // Lê o arquivo de entrada, codificando símbolo por símbolo.
    int c;
    while ((c = fgetc(input)) != EOF) encode_symbol(output, c);
    
    // Codifica o símbolo de fim de arquivo para o descompressor saber onde parar.
    encode_symbol(output, EOT_SYMBOL);
    flush_bits(output); // Garante que o último byte seja escrito.
    
    fclose(input); fclose(output);
    printf("Arquivo compactado com sucesso.\n");
}

// Função principal para descompressão de um arquivo.
void decompressFile(const char* input_path, const char* output_path) {
    FILE* input = fopen(input_path, "rb");
    FILE* output = fopen(output_path, "wb");
    if (!input || !output) { perror("Erro ao abrir arquivos"); return; }
    
    initialize_tree();
    Node* curr = root; // Começa a navegação pela raiz.
    while (1) {
        if (curr->left == NULL && curr->right == NULL) { // Chegou a uma folha.
            int symbol = curr->symbol;
            // Se a folha for NYT, lê o símbolo literal do arquivo.
            if (symbol == INTERNAL_NODE) {
                symbol = 0;
                // Lê 9 bits para reconstruir o novo símbolo.
                for (int i = 0; i < 9; i++) {
                    int bit = read_bit(input);
                    if (bit == EOF) goto cleanup;
                    symbol = (symbol << 1) | bit;
                }
            }
            // Se for o símbolo EOT, termina a descompressão.
            if (symbol == EOT_SYMBOL) break;
            
            fputc(symbol, output);
            update_tree(symbol); // Atualiza a árvore, assim como o compressor fez.
            curr = root; // Volta para a raiz.
        } else {
            int bit = read_bit(input);
            if (bit == EOF) break;
            curr = (bit == 0) ? curr->left : curr->right;
        }
    }
cleanup: // Ponto de saída para fechar os arquivos.
    fclose(input); fclose(output);
    printf("Arquivo descompactado com sucesso.\n");
}
