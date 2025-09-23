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

const int SIDEBAR_W = 420;
const int SIDEBAR_H = 480;

// Espaçamento entre janelas
const int GAP = 12;

// Botão
SDL_FRect buttonRect = {20.0f, SIDEBAR_H - 70.0f, 140.0f, 40.0f};

enum ButtonState { BTN_NEUTRAL, BTN_HOVER, BTN_ACTIVE };

// Conversão para luminância
inline Uint8 luminanceUint8(Uint8 r, Uint8 g, Uint8 b) {
    double y = 0.2125 * r + 0.7154 * g + 0.0721 * b;
    if (y < 0) y = 0; if (y > 255) y = 255;
    return (Uint8) (y + 0.5);
}

// Calcula histograma
void computeHistogram(const std::vector<Uint8>& gray, int w, int h, std::vector<int>& hist) {
    hist.assign(256, 0);
    for (size_t i = 0; i < gray.size(); ++i) hist[gray[i]]++;
}

// Calcula média e desvio padrão
void computeMeanStd(const std::vector<int>& hist, int totalPixels, double &mean, double &stddev) {
    mean = 0.0; stddev = 0.0;
    for (int i = 0; i < 256; ++i) mean += i * hist[i];
    mean /= totalPixels;
    for (int i = 0; i < 256; ++i) stddev += hist[i] * (i - mean) * (i - mean);
    stddev = std::sqrt(stddev / totalPixels);
}

// Equaliza o histograma
void equalizeHistogram(std::vector<Uint8>& gray, const std::vector<int>& hist, int totalPixels) {
    std::vector<double> cdf(256, 0.0);
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i) cdf[i] = cdf[i-1] + hist[i];
    
    double cdf_min = 0;
    for(int i = 0; i < 256; ++i) {
        if(hist[i] > 0) {
            cdf_min = cdf[i-1];
            break;
        }
    }

    double denominator = totalPixels - cdf_min;
    if (denominator <= 0) denominator = 1;

    for (int i = 0; i < 256; ++i) cdf[i] = (cdf[i] - cdf_min) / denominator;
    
    std::vector<Uint8> lut(256);
    for (int i = 0; i < 256; ++i) lut[i] = (Uint8)std::round(cdf[i] * 255.0);
    for (size_t k = 0; k < gray.size(); ++k) {
        gray[k] = lut[gray[k]];
    }
}

// Cria uma SDL_Surface
SDL_Surface* createSurfaceFromGray(const std::vector<Uint8>& gray, int w, int h) {
    SDL_Surface* surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return nullptr;
    
    Uint32* pixels = (Uint32*) surf->pixels;
    const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(surf->format);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Uint8 g = gray[y * w + x];
            pixels[y * (surf->pitch/4) + x] = SDL_MapRGBA(details, NULL, g, g, g, 255);
        }
    }
    return surf;
}

// Renderiza histograma
void renderHistogram(SDL_Renderer* renderer, const std::vector<int>& hist) {
    int w, h;
    SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
    float margin = 10.0f;
    float gx = margin, gy = margin;
    float gw = w - 2*margin, gh = h - 145.0f; 

    int maxv = 1;
    for (int v : hist) if (v > maxv) maxv = v;

    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < 256; ++i) {
        double frac = (double)hist[i] / maxv;
        float barW = std::max(1.0f, gw / 256.0f);
        float x = gx + i * barW;
        float barH = (float) (frac * gh);
        
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_FRect r = {x, gy + (gh - barH), barW, barH};
        SDL_RenderFillRect(renderer, &r);
    }
}

std::string classifyMean(double mean) {
    if (mean >= 170.0) return "clara";
    if (mean <= 85.0) return "escura";
    return "média";
}

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

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Surface* loaded = IMG_Load(imagePath);
    if (!loaded) {
        printf("Nao conseguiu carregar imagem '%s' : %s\n", imagePath, SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    int imgW = loaded->w, imgH = loaded->h;
    SDL_Surface* conv = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA8888);
    SDL_DestroySurface(loaded);
    if (!conv) {
        printf("Erro ao converter formato: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    std::vector<Uint8> gray(imgW * imgH);
    Uint8* pixels = (Uint8*) conv->pixels;
    int pitch = conv->pitch;
    for (int y = 0; y < imgH; ++y) {
        Uint8* row = pixels + y * pitch;
        for (int x = 0; x < imgW; ++x) {
            Uint8 r = row[x*4 + 0];
            Uint8 g = row[x*4 + 1];
            Uint8 b = row[x*4 + 2];
            gray[y*imgW + x] = luminanceUint8(r,g,b);
        }
    }
    SDL_DestroySurface(conv);

    std::vector<Uint8> grayOriginal = gray;
    std::vector<int> hist(256);
    computeHistogram(gray, imgW, imgH, hist);

    SDL_Rect displayBounds;
    SDL_GetDisplayUsableBounds(0, &displayBounds);

    int mainX = displayBounds.x + (displayBounds.w - imgW - SIDEBAR_W - GAP) / 2;
    int mainY = displayBounds.y + (displayBounds.h - std::max(imgH, SIDEBAR_H)) / 2;
    if (mainY < displayBounds.y) { mainY = displayBounds.y; }

    
    SDL_Window* winMain = SDL_CreateWindow("Visualizador - Imagem", imgW, imgH, 0);
    SDL_SetWindowPosition(winMain, mainX, mainY);

    SDL_Window* winSide = SDL_CreateWindow("Histograma e Controles", SIDEBAR_W, SIDEBAR_H, 0);
    SDL_SetWindowPosition(winSide, mainX + imgW + GAP, mainY);
    
    SDL_Renderer* renMain = SDL_CreateRenderer(winMain, NULL);
    SDL_Renderer* renSide = SDL_CreateRenderer(winSide, NULL);
    TTF_Font* font = TTF_OpenFont("font.ttf", 18);
    if (!font) {
        printf("Aviso: Nao encontrou 'font.ttf'. As informações de texto não serão exibidas. Erro: %s\n", SDL_GetError());
    }

    SDL_Surface* surf = createSurfaceFromGray(gray, imgW, imgH);
    SDL_Texture* texMain = SDL_CreateTextureFromSurface(renMain, surf);

    bool running = true;
    bool isEqualized = false;
    ButtonState bstate = BTN_NEUTRAL;

    double mean = 0.0, stddev = 0.0;
    computeMeanStd(hist, imgW*imgH, mean, stddev);
    
    SDL_Color black = {0, 0, 0, 255};

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.scancode == SDL_SCANCODE_S) {
                    if (IMG_SavePNG(surf, "output_image.png") == 0) {
                        printf("Sucesso: saved output_image.png\n");
                    } else {
                        fprintf(stderr, "Erro ao salvar: %s\n", SDL_GetError());
                    }
                }
            } else if (e.type == SDL_EVENT_MOUSE_MOTION) {
                if (e.motion.windowID == SDL_GetWindowID(winSide)) {
                    SDL_FPoint mousePoint = {e.motion.x, e.motion.y};
                    if (SDL_PointInRectFloat(&mousePoint, &buttonRect)) {
                        bstate = BTN_HOVER;
                    } else {
                        bstate = BTN_NEUTRAL;
                    }
                }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                 if (e.button.windowID == SDL_GetWindowID(winSide)) {
                    SDL_FPoint mousePoint = {e.button.x, e.button.y};
                    if (SDL_PointInRectFloat(&mousePoint, &buttonRect)) {
                        bstate = BTN_ACTIVE;
                    }
                }
            } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (e.button.windowID == SDL_GetWindowID(winSide)) {
                    SDL_FPoint mousePoint = {e.button.x, e.button.y};
                    if (SDL_PointInRectFloat(&mousePoint, &buttonRect)) {
                        isEqualized = !isEqualized;
                        if (isEqualized) {
                            equalizeHistogram(gray, hist, imgW*imgH);
                        } else {
                            gray = grayOriginal;
                        }
                        computeHistogram(gray, imgW, imgH, hist);
                        SDL_DestroySurface(surf);
                        surf = createSurfaceFromGray(gray, imgW, imgH);
                        SDL_DestroyTexture(texMain);
                        texMain = SDL_CreateTextureFromSurface(renMain, surf);
                        computeMeanStd(hist, imgW*imgH, mean, stddev);
                    }
                }
                bstate = BTN_NEUTRAL;
            }
        }

        SDL_SetRenderDrawColor(renMain, 0,0,0,255);
        SDL_RenderClear(renMain);
        SDL_RenderTexture(renMain, texMain, NULL, NULL);
        SDL_RenderPresent(renMain);

        renderHistogram(renSide, hist);
        if (font) {
            char buf[256];
            snprintf(buf, sizeof(buf), "Media: %.2f (%s)", mean, classifyMean(mean).c_str());
            SDL_Surface* t1 = TTF_RenderText_Blended(font, buf, 0, black);
            SDL_Texture* tt1 = SDL_CreateTextureFromSurface(renSide, t1);
            SDL_FRect r1 = {10.0f, SIDEBAR_H - 125.0f, (float)t1->w, (float)t1->h};
            SDL_RenderTexture(renSide, tt1, NULL, &r1);
            SDL_DestroyTexture(tt1); SDL_DestroySurface(t1);

            char buf2[256];
            snprintf(buf2, sizeof(buf2), "Desvio: %.2f (%s)", stddev, classifyStd(stddev).c_str());
            SDL_Surface* t2 = TTF_RenderText_Blended(font, buf2, 0, black);
            SDL_Texture* tt2 = SDL_CreateTextureFromSurface(renSide, t2);
            SDL_FRect r2 = {10.0f, SIDEBAR_H - 100.0f, (float)t2->w, (float)t2->h};
            SDL_RenderTexture(renSide, tt2, NULL, &r2);
            SDL_DestroyTexture(tt2); SDL_DestroySurface(t2);
        }

        if (bstate == BTN_NEUTRAL) SDL_SetRenderDrawColor(renSide, 180,180,180,255);
        else if (bstate == BTN_HOVER) SDL_SetRenderDrawColor(renSide, 150,200,255,255);
        else SDL_SetRenderDrawColor(renSide, 100,160,220,255);
        SDL_RenderFillRect(renSide, &buttonRect);
        
        SDL_SetRenderDrawColor(renSide, 40,40,40,255);
        SDL_RenderRect(renSide, &buttonRect);

        if (font) {
            const char* label = isEqualized ? "Original" : "Equalizar";
            SDL_Surface* tb = TTF_RenderText_Blended(font, label, 0, black);
            SDL_Texture* tbtx = SDL_CreateTextureFromSurface(renSide, tb);
            SDL_FRect tr = { buttonRect.x + (buttonRect.w - tb->w)/2.0f, buttonRect.y + (buttonRect.h - tb->h)/2.0f, (float)tb->w, (float)tb->h };
            SDL_RenderTexture(renSide, tbtx, NULL, &tr);
            SDL_DestroyTexture(tbtx); SDL_DestroySurface(tb);
        }

        SDL_RenderPresent(renSide);
        SDL_Delay(16);
    }

    SDL_DestroyTexture(texMain);
    SDL_DestroySurface(surf);
    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(renMain); SDL_DestroyRenderer(renSide);
    SDL_DestroyWindow(winMain); SDL_DestroyWindow(winSide);
    TTF_Quit(); 
    SDL_Quit();

    return 0;
}
