// sdl_image_processor.cpp
// Compilável com: g++ -std=c++17 sdl_image_processor.cpp -o sdl_image_processor \
//    `sdl3-config --cflags --libs` -lSDL3_image -lSDL3_ttf
// Obs: coloque um arquivo de fonte TrueType chamado "font.ttf" no mesmo diretório do executável

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

// Configurações da janela secundária (tamanho fixo)
const int SIDEBAR_W = 420;
const int SIDEBAR_H = 480;

// Espaçamento entre janelas
const int GAP = 12;

// Botão
SDL_Rect buttonRect = {20, SIDEBAR_H - 70, 140, 40};

enum ButtonState { BTN_NEUTRAL, BTN_HOVER, BTN_ACTIVE };

// Conversão para luminância
inline Uint8 luminanceUint8(Uint8 r, Uint8 g, Uint8 b) {
    double y = 0.2125 * r + 0.7154 * g + 0.0721 * b;
    if (y < 0) y = 0; if (y > 255) y = 255;
    return (Uint8) (y + 0.5);
}

// Calcula histograma (256 bins) a partir de buffer de tons de cinza
void computeHistogram(const std::vector<Uint8>& gray, int w, int h, std::vector<int>& hist) {
    hist.assign(256, 0);
    for (size_t i = 0; i < gray.size(); ++i) hist[gray[i]]++;
}

// Calcula média e desvio padrão a partir do histograma
void computeMeanStd(const std::vector<int>& hist, int totalPixels, double &mean, double &stddev) {
    mean = 0.0; stddev = 0.0;
    for (int i = 0; i < 256; ++i) mean += i * hist[i];
    mean /= totalPixels;
    for (int i = 0; i < 256; ++i) stddev += hist[i] * (i - mean) * (i - mean);
    stddev = std::sqrt(stddev / totalPixels);
}

// Equaliza o histograma (in-place no buffer gray)
void equalizeHistogram(std::vector<Uint8>& gray, const std::vector<int>& hist, int totalPixels) {
    std::vector<double> cdf(256);
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i) cdf[i] = cdf[i-1] + hist[i];
    for (int i = 0; i < 256; ++i) cdf[i] = (cdf[i] - cdf[0]) / (totalPixels - cdf[0]);
    // Map
    std::vector<Uint8> lut(256);
    for (int i = 0; i < 256; ++i) lut[i] = (Uint8)std::round(cdf[i] * 255.0);
    for (size_t k = 0; k < gray.size(); ++k) {
        gray[k] = lut[gray[k]];
    }
}

// Cria uma SDL_Surface RGBA a partir de buffer gray (R=G=B=gray, A=255)
SDL_Surface* createSurfaceFromGray(const std::vector<Uint8>& gray, int w, int h) {
    SDL_Surface* surf = SDL_CreateSurface((SDL_PixelFormatEnum)SDL_PIXELFORMAT_RGBA8888, w, h, 32);
    if (!surf) return nullptr;
    // Preenchendo pixels
    Uint32* pixels = (Uint32*) surf->pixels;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Uint8 g = gray[y * w + x];
            Uint32 rgba = (255u << 24) | (g << 16) | (g << 8) | g; // A R G B (sdl uses native order but this is portable with RGBA8888)
            pixels[y * (surf->pitch/4) + x] = rgba;
        }
    }
    return surf;
}

// Renderiza histograma como barras na janela secundaria (renderer passado)
void renderHistogram(SDL_Renderer* renderer, const std::vector<int>& hist) {
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    int margin = 10;
    int gx = margin, gy = margin;
    int gw = w - 2*margin, gh = h - 110; // reservar espaço para texto e botão

    int maxv = 1;
    for (int v : hist) if (v > maxv) maxv = v;

    // fundo
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    SDL_RenderFillRect(renderer, NULL);

    // barras
    for (int i = 0; i < 256; ++i) {
        double frac = (double)hist[i] / maxv;
        int barW = std::max(1, gw / 256);
        int x = gx + i * barW;
        int barH = (int) (frac * gh);
        // desenha barra
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_Rect r = {x, gy + (gh - barH), barW, barH};
        SDL_RenderFillRect(renderer, &r);
    }
}

// Classifica média em clara/média/escura
std::string classifyMean(double mean) {
    if (mean >= 170.0) return "clara";
    if (mean <= 85.0) return "escura";
    return "média";
}

// Classifica desvio padrão em alto/médio/baixo
std::string classifyStd(double stddev) {
    if (stddev >= 60.0) return "alto";
    if (stddev <= 20.0) return "baixo";
    return "médio";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s <imagem>", argv[0]);
        return 1;
    }
    const char* imagePath = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init erro: %s\n", SDL_GetError());
        return 1;
    }
    if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)) {
        printf("IMG_Init falhou: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() != 0) {
        printf("TTF_Init erro: %s\n", TTF_GetError());
        IMG_Quit(); SDL_Quit();
        return 1;
    }

    // Carrega surface original via SDL_image
    SDL_Surface* loaded = IMG_Load(imagePath);
    if (!loaded) {
        printf("Nao conseguiu carregar imagem '%s' : %s\n", imagePath, IMG_GetError());
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    int imgW = loaded->w, imgH = loaded->h;
    // Obter pixels em formato conhecido (RGBA8888)
    SDL_Surface* conv = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_RGBA8888, 0);
    SDL_FreeSurface(loaded);
    if (!conv) {
        printf("Erro ao converter formato: %s\n", SDL_GetError());
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    // Extrair buffer grayscale
    std::vector<Uint8> gray(imgW * imgH);
    Uint8* pixels = (Uint8*) conv->pixels;
    int pitch = conv->pitch; // bytes per row
    for (int y = 0; y < imgH; ++y) {
        Uint8* row = pixels + y * pitch;
        for (int x = 0; x < imgW; ++x) {
            Uint8 r = row[x*4 + 0];
            Uint8 g = row[x*4 + 1];
            Uint8 b = row[x*4 + 2];
            gray[y*imgW + x] = luminanceUint8(r,g,b);
        }
    }
    SDL_FreeSurface(conv);

    // Salva uma cópia original do gray para reverter
    std::vector<Uint8> grayOriginal = gray;

    // Calcula histograma
    std::vector<int> hist(256);
    computeHistogram(gray, imgW, imgH, hist);

    // Criar janelas
    int displayIndex = 0;
    SDL_Rect displayBounds;
    SDL_GetDisplayUsableBounds(displayIndex, &displayBounds);

    int mainX = displayBounds.x + (displayBounds.w - imgW - SIDEBAR_W - GAP) / 2;
    int mainY = displayBounds.y + (displayBounds.h - std::max(imgH, SIDEBAR_H)) / 2;
    if (mainX < displayBounds.x) mainX = displayBounds.x + 50;
    if (mainY < displayBounds.y) mainY = displayBounds.y + 50;

    SDL_Window* winMain = SDL_CreateWindow("Visualizador - Imagem", mainX, mainY, imgW, imgH, SDL_WINDOW_SHOWN);
    if (!winMain) { printf("Erro criando janela principal: %s\n", SDL_GetError()); return 1; }

    SDL_Window* winSide = SDL_CreateWindow("Histograma e Controles", mainX + imgW + GAP, mainY, SIDEBAR_W, SIDEBAR_H, SDL_WINDOW_SHOWN);
    if (!winSide) { printf("Erro criando janela secundaria: %s\n", SDL_GetError()); SDL_DestroyWindow(winMain); return 1; }

    SDL_Renderer* renMain = SDL_CreateRenderer(winMain, NULL);
    SDL_Renderer* renSide = SDL_CreateRenderer(winSide, NULL);
    if (!renMain || !renSide) { printf("Erro criando renderers: %s\n", SDL_GetError()); SDL_DestroyWindow(winMain); SDL_DestroyWindow(winSide); return 1; }

    // Carregar fonte (arquivo font.ttf requerido no mesmo dir)
    TTF_Font* font = TTF_OpenFont("font.ttf", 18);
    if (!font) {
        printf("Nao encontrou 'font.ttf'. Coloque um arquivo de fonte TrueType chamado font.ttf no diretorio do executavel. TTF_Error: %s\n", TTF_GetError());
        // mas continua: criaremos sem texto rederizado se nao houver fonte
    }

    // Criar textura da imagem inicialmente
    SDL_Surface* surf = createSurfaceFromGray(gray, imgW, imgH);
    if (!surf) { printf("Erro criando surface de gray\n"); return 1; }
    SDL_Texture* texMain = SDL_CreateTextureFromSurface(renMain, surf);
    if (!texMain) { printf("Erro criando texture principal: %s\n", SDL_GetError()); }

    bool running = true;
    bool isEqualized = false;
    ButtonState bstate = BTN_NEUTRAL;

    // Variaveis para texto estatistico
    double mean = 0.0, stddev = 0.0;
    computeMeanStd(hist, imgW*imgH, mean, stddev);

    // Loop
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.keysym.sym == SDL_KEYCODE_S) {
                    // salvar a surf atual em PNG
                    if (IMG_SavePNG(surf, "output_image.png") == 0) { // Checa explicitamente por 0 (sucesso)
                        printf("Sucesso: saved output_image.png\n");
                    } else {
                        fprintf(stderr, "Erro ao salvar: %s\n", SDL_GetError());
                    }
                }
            } else if (e.type == SDL_EVENT_MOUSE_MOTION) {
                int mx = e.motion.x - 0; // posição relativa à janela em que o evento ocorreu
                int my = e.motion.y - 0;
                // eventos são gerados por janelas diferentes; precisamos checar windowID
                Uint32 winID = e.motion.windowID;
                Uint32 sideID = SDL_GetWindowID(winSide);
                if (winID == sideID) {
                    // coordenadas relativas à janela secundária
                    int rx = e.motion.x;
                    int ry = e.motion.y;
                    if (rx >= buttonRect.x && rx <= buttonRect.x + buttonRect.w && ry >= buttonRect.y && ry <= buttonRect.y + buttonRect.h)
                        bstate = BTN_HOVER;
                    else bstate = BTN_NEUTRAL;
                }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (e.button.windowID == SDL_GetWindowID(winSide)) {
                    int rx = e.button.x;
                    int ry = e.button.y;
                    if (rx >= buttonRect.x && rx <= buttonRect.x + buttonRect.w && ry >= buttonRect.y && ry <= buttonRect.y + buttonRect.h) {
                        bstate = BTN_ACTIVE;
                    }
                }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (e.button.windowID == SDL_GetWindowID(winSide)) {
                    int rx = e.button.x;
                    int ry = e.button.y;
                    if (rx >= buttonRect.x && rx <= buttonRect.x + buttonRect.w && ry >= buttonRect.y && ry <= buttonRect.y + buttonRect.h) {
                        // clicar: alterna equaliza/reverte
                        if (!isEqualized) {
                            // aplicar equalizacao
                            equalizeHistogram(gray, hist, imgW*imgH);
                            isEqualized = true;
                        } else {
                            gray = grayOriginal;
                            isEqualized = false;
                        }
                        // atualizar histograma, surface e textura
                        computeHistogram(gray, imgW, imgH, hist);
                        SDL_FreeSurface(surf);
                        surf = createSurfaceFromGray(gray, imgW, imgH);
                        if (texMain) SDL_DestroyTexture(texMain);
                        texMain = SDL_CreateTextureFromSurface(renMain, surf);
                        computeMeanStd(hist, imgW*imgH, mean, stddev);
                    }
                    bstate = BTN_NEUTRAL;
                }
            }
        }

        // Renderizar janela principal
        SDL_SetRenderTarget(renMain, NULL);
        SDL_SetRenderDrawColor(renMain, 0,0,0,255);
        SDL_RenderClear(renMain);
        if (texMain) {
            SDL_Rect dst = {0,0,imgW,imgH};
            SDL_RenderCopy(renMain, texMain, NULL, &dst);
        }
        SDL_RenderPresent(renMain);

        // Renderizar janela secundaria
        SDL_SetRenderTarget(renSide, NULL);
        renderHistogram(renSide, hist);

        // desenhar texto com media e desvio (usando TTF se disponivel)
        if (font) {
            char buf[256];
            snprintf(buf, sizeof(buf), "Media: %.2f (%s)", mean, classifyMean(mean).c_str());
            SDL_Surface* t1 = TTF_RenderUTF8_Blended(font, buf, {0,0,0,255});
            SDL_Texture* tt1 = SDL_CreateTextureFromSurface(renSide, t1);
            SDL_Rect r1 = {10, SIDEBAR_H - 125, t1->w, t1->h};
            SDL_RenderCopy(renSide, tt1, NULL, &r1);
            SDL_DestroyTexture(tt1); SDL_FreeSurface(t1);

            char buf2[256];
            snprintf(buf2, sizeof(buf2), "Desvio: %.2f (%s)", stddev, classifyStd(stddev).c_str());
            SDL_Surface* t2 = TTF_RenderUTF8_Blended(font, buf2, {0,0,0,255});
            SDL_Texture* tt2 = SDL_CreateTextureFromSurface(renSide, t2);
            SDL_Rect r2 = {10, SIDEBAR_H - 100, t2->w, t2->h};
            SDL_RenderCopy(renSide, tt2, NULL, &r2);
            SDL_DestroyTexture(tt2); SDL_FreeSurface(t2);
        }

        // desenhar botao
        if (bstate == BTN_NEUTRAL) SDL_SetRenderDrawColor(renSide, 180,180,180,255);
        else if (bstate == BTN_HOVER) SDL_SetRenderDrawColor(renSide, 150,200,255,255);
        else SDL_SetRenderDrawColor(renSide, 100,160,220,255);
        SDL_RenderFillRect(renSide, &buttonRect);
        // borda do botao
        SDL_SetRenderDrawColor(renSide, 40,40,40,255);
        SDL_RenderDrawRect(renSide, &buttonRect);

        // Desenhar texto do botao
        if (font) {
            const char* label = isEqualized ? "Original" : "Equalizar";
            SDL_Surface* tb = TTF_RenderUTF8_Blended(font, label, {0,0,0,255});
            SDL_Texture* tbtx = SDL_CreateTextureFromSurface(renSide, tb);
            SDL_Rect tr = { buttonRect.x + (buttonRect.w - tb->w)/2, buttonRect.y + (buttonRect.h - tb->h)/2, tb->w, tb->h };
            SDL_RenderCopy(renSide, tbtx, NULL, &tr);
            SDL_DestroyTexture(tbtx); SDL_FreeSurface(tb);
        }

        SDL_RenderPresent(renSide);

        SDL_Delay(16);
    }

    if (texMain) SDL_DestroyTexture(texMain);
    if (surf) SDL_FreeSurface(surf);
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renMain); SDL_DestroyRenderer(renSide);
    SDL_DestroyWindow(winMain); SDL_DestroyWindow(winSide);
    TTF_Quit(); IMG_Quit(); SDL_Quit();

    return 0;
}
