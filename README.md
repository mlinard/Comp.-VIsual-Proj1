# Proj1 - Processamento de Imagens em Escala de Cinza

**Universidade Presbiteriana Mackenzie**
**Faculdade de Computação e Informática - Ciência da Computação**
**Disciplina:** Computação Visual
**Professor:** André Kishimoto

## Descrição do Projeto

Este projeto consiste em um software de processamento de imagens desenvolvido como parte da disciplina de Computação Visual. O programa, escrito em C/C++, utiliza a biblioteca `SDL3` para carregar uma imagem, convertê-la para escala de cinza, analisar seu histograma e permitir a equalização desse histograma através de uma interface gráfica simples.

O objetivo é aplicar conceitos de manipulação de pixels, análise de dados de imagem (histograma) e o uso de bibliotecas gráficas para criar uma ferramenta funcional. O programa é executado via linha de comando e exibe a imagem processada e suas informações em duas janelas separadas.

## Integrantes do Grupo

| Nome Completo                      | RA       |
| ---------------------------------- | -------- |
| Marcello Linard Teixeira           | 10419338 |
| Felipe José de Castro              | 10356965 |
| Mauricio Gabriel Gutirrez Garcia   | 10403130 |

## Como o projeto funciona

O software opera em uma série de etapas bem definidas:

1.  **Inicialização:** Ao ser executado com o caminho de uma imagem como argumento, o programa inicializa a biblioteca `SDL3` e a `SDL_image`.
2.  **Carregamento e Validação:** A imagem fornecida é carregada na memória. O sistema realiza o tratamento de erros para casos como arquivo não encontrado ou formato de imagem inválido.
3.  **Conversão para Escala de Cinza:** O programa verifica se a imagem é colorida ou se já está em escala de cinza. Se for colorida, cada pixel é convertido usando a fórmula de luminância $Y=0.2125*R+0.7154*G+0.0721*B$. Esta imagem em tons de cinza serve como base para todas as operações subsequentes.
4.  **Criação da Interface Gráfica:** Duas janelas são criadas:
    * **Janela Principal:** Exibe a imagem processada. Seu tamanho é adaptado ao da imagem e ela aparece centralizada no monitor principal.
    * **Janela Secundária:** Uma janela filha de tamanho fixo, posicionada ao lado da principal. Ela contém a visualização do histograma e o botão de operação.
5.  **Análise e Exibição do Histograma:** O histograma dos 256 tons de cinza é calculado e desenhado na janela secundária. Com base na média e no desvio padrão dos níveis de intensidade, o programa classifica a imagem como "clara", "média" ou "escura" e seu contraste como "alto", "médio" ou "baixo".
6.  **Equalização do Histograma:** Um botão na janela secundária permite ao usuário aplicar a equalização do histograma. Ao ser clicado, a imagem na janela principal e o histograma na secundária são atualizados. Um segundo clique reverte a imagem para a versão original em escala de cinza. O botão também fornece feedback visual para o usuário (mouse sobre, clique) e seu texto é alterado para refletir a ação.
7.  **Salvar Imagem:** Ao pressionar a tecla 'S', a imagem atualmente exibida na janela principal é salva no arquivo `output_image.png`, sobrescrevendo-o caso já exista.

## Como compilar e executar

O projeto foi desenvolvido para ser compilado com `gcc` ou `g++` em um ambiente com as bibliotecas `SDL3` e `SDL_image` devidamente instaladas.

**Pré-requisitos:**
* Compilador C/C++ (`gcc` ou `g++`)
* Biblioteca `SDL3`
* Biblioteca `SDL_image`

**Passos para compilação:**

```bash
# Clone o repositório
git clone [URL_DO_REPOSITÓRIO]
cd [NOME_DA_PASTA_DO_PROJETO]

# Exemplo de comando de compilação 
# Para C++:
g++ src/*.cpp -o programa -lSDL3 -lSDL3_image

# Para C:
gcc src/*.c -o programa -lSDL3 -lSDL3_image
