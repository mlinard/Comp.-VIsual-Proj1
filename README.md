# Proj1 - Processamento de Imagens em Escala de Cinza

**Universidade Presbiteriana Mackenzie** **Faculdade de Computação e Informática - Ciência da Computação** **Disciplina:** Computação Visual  
**Professor:** André Kishimoto

## Descrição do Projeto

Este projeto consiste em um software de processamento de imagens desenvolvido como parte da disciplina de Computação Visual. O programa, escrito em C++, utiliza a biblioteca `SDL3` e suas extensões (`SDL_image` e `SDL_ttf`) para carregar uma imagem, convertê-la para escala de cinza, analisar seu histograma e permitir a equalização desse histograma, tudo através de uma interface gráfica interativa.

O objetivo é aplicar conceitos de manipulação de pixels, análise de dados de imagem e o uso de bibliotecas gráficas para criar uma ferramenta funcional. O programa é executado via linha de comando e exibe a imagem processada e suas informações em duas janelas separadas.

## Integrantes do Grupo

| Nome Completo                      | RA       |
| ---------------------------------- | -------- |
| Marcello Linard Teixeira           | 10419338 |
| Felipe José de Castro              | 10356965 |
| Mauricio Gabriel Gutirrez Garcia   | 10403130 |

## Funcionalidades Implementadas

* **Carregamento de Imagem:** O programa carrega imagens nos formatos PNG, JPG e BMP, com tratamento de erros para arquivos não encontrados ou formatos inválidos.
* **Conversão para Escala de Cinza:** Analisa se a imagem é colorida e, em caso afirmativo, a converte para tons de cinza usando a fórmula de luminância $Y=0.2125*R+0.7154*G+0.0721*B$.
* **Interface Gráfica com Duas Janelas:**
    * Uma janela principal que se adapta ao tamanho da imagem e exibe o resultado do processamento.
    * Uma janela secundária de tamanho fixo que exibe o histograma e os controles da aplicação.
* **Análise e Exibição do Histograma:**
    * Cálculo e exibição gráfica do histograma da imagem em escala de cinza.
    * Análise da **média de intensidade** (classificando a imagem como "clara", "média" ou "escura") e do **desvio padrão** (classificando o contraste como "alto", "médio" ou "baixo").
* **Equalização de Histograma Interativa:**
    * Um botão na janela secundária permite ao usuário equalizar o histograma da imagem.
    * Clicar novamente no botão reverte a imagem para a versão original em escala de cinza.
    * O botão possui feedback visual para o usuário (cores para estado neutro, hover e clique) e seu texto muda para refletir a ação disponível.
* **Salvamento da Imagem:** Ao pressionar a tecla 'S', a imagem atualmente exibida é salva no arquivo `output_image.png`, sobrescrevendo qualquer arquivo existente.

## Como Compilar e Executar

Este projeto foi desenvolvido e testado no **Windows 10/11** utilizando o compilador **MinGW-w64 (g++)**.

### Pré-requisitos de Ambiente
1.  **Compilador:** Um compilador C++17 ou superior. Recomendamos o **MinGW-w64**, que pode ser instalado via [MSYS2](https://www.msys2.org/).
2.  **Bibliotecas SDL3 (Development Libraries para MinGW):**
    * `SDL3`
    * `SDL3_image`
    * `SDL3_ttf`

### Estrutura de Pastas
Para que a compilação funcione, os arquivos do projeto devem ser organizados na seguinte estrutura:
CompVisual/
|-- sdl_image_processor.cpp
|-- font.ttf
|
|-- SDL3-devel-[versao]-mingw/
|-- SDL3_image-devel-[versao]-mingw/
`-- SDL3_ttf-devel-[versao]-mingw/

### Comando de Compilação
Abra o terminal **MSYS2 MINGW64**, navegue até a pasta raiz do projeto (`CompVisual`) e execute o seguinte comando:

```bash
g++ -std=c++17 -g sdl_image_processor.cpp -I ./SDL3-devel-[versao]-mingw/x86_64-w64-mingw32/include -I ./SDL3_image-devel-[versao]-mingw/x86_64-w64-mingw32/include -I ./SDL3_ttf-devel-[versao]-mingw/x86_64-w64-mingw32/include -L ./SDL3-devel-[versao]-mingw/x86_64-w64-mingw32/lib -L ./SDL3_image-devel-[versao]-mingw/x86_64-w64-mingw32/lib -L ./SDL3_ttf-devel-[versao]-mingw/x86_64-w64-mingw32/lib -o programa.exe -lmingw32 -lSDL3 -lSDL3_image -lSDL3_ttf
