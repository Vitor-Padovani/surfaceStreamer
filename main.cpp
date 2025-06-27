// include basic libraries
#include <iostream>
#include <thread>
#include <math.h>
#include <vector>
#include <chrono>

// include igtlink headers 
#include "igtlOSUtil.h"
#include "igtlMessageHeader.h"
#include "igtlServerSocket.h"
#include "igtlImageMessage.h"

const double M_PI = 3.1415926535;

const int PLANE_WIDTH = 300;
const int PLANE_HEIGHT = 300;

std::atomic_bool StopFlag = false;

// Vectors para armazenar todos os frames pré-calculados
std::vector<std::vector<std::vector<float>>> preCalculatedFrames;

// Dimensões da imagem (mais quadrada possível)
int imageWidth, imageHeight;

// Criar e configurar timestamp
igtl::TimeStamp::Pointer timeStamp = igtl::TimeStamp::New();

// Função para calcular dimensões mais quadradas possível
void CalculateImageDimensions(int totalPoints)
{
    // Encontrar dimensões mais próximas de um quadrado
    imageWidth = static_cast<int>(ceil(sqrt(totalPoints)));
    imageHeight = static_cast<int>(ceil(static_cast<double>(totalPoints) / imageWidth));

    std::cout << "Total points: " << totalPoints << std::endl;
    std::cout << "Image dimensions: " << imageWidth << " x " << imageHeight << std::endl;
    std::cout << "Total pixels: " << (imageWidth * imageHeight) << std::endl;
}

// Função que gera um plano de pontos
std::vector<std::vector<float>> GeneratePlanePoints(
    int width, int height, float spacing = 1.0f, float z_level = 0.0f, float offset = -100.0f)
{
    std::vector<std::vector<float>> points;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            std::vector<float> point(3);
            point[0] = (j * spacing) + offset;  // X coordinate
            point[1] = (i * spacing) + offset;  // Y coordinate
            point[2] = z_level;      // Z coordinate (plano)
            points.push_back(point);
        }
    }

    return points;
}

// Função que aplica efeito wave aos pontos do plano
std::vector<std::vector<float>> ApplyWaveEffect(
    const std::vector<std::vector<float>>& originalPoints,
    int width,
    int height,
    float time,
    float amplitude,
    float frequency,
    float waveSpeed)
{
    std::vector<std::vector<float>> wavePoints = originalPoints;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int index = i * width + j;

            // Calcular coordenadas normalizadas (0 a 1)
            float normalizedX = (float)j / (width - 1);
            float normalizedY = (float)i / (height - 1);

            // Aplicar múltiplas ondas para efeito mais interessante
            float wave1 = sin(2.0f * M_PI * frequency * normalizedX + waveSpeed * time);
            float wave2 = cos(2.0f * M_PI * frequency * normalizedY + waveSpeed * time);
            float wave3 = sin(2.0f * M_PI * frequency * (normalizedX + normalizedY) + waveSpeed * time * 0.7f);

            // Combinar as ondas
            float combinedWave = (wave1 + wave2 + wave3 * 0.5f) / 2.5f;

            // Aplicar amplitude e atualizar coordenada Z
            wavePoints[index][2] = amplitude * combinedWave;
        }
    }

    return wavePoints;
}

// Função que pré-calcula todos os frames do wave effect
void PreCalculateWaveFrames(int width, int height, int numFrames, float frameRate = 30.0f)
{
    std::cout << "Pre-calculating " << numFrames << " wave frames..." << std::endl;

    // Calcular dimensões da imagem
    CalculateImageDimensions(width * height);

    // Limpar frames anteriores
    preCalculatedFrames.clear();
    preCalculatedFrames.reserve(numFrames);

    // Gerar plano base
    std::vector<std::vector<float>> basePlanePoints = GeneratePlanePoints(width, height, 2.0f, 0.0f);

    // Pré-calcular todos os frames
    for (int frame = 0; frame < numFrames; frame++)
    {
        float time = frame / frameRate; // Tempo em segundos

        // Aplicar wave effect para este frame
        std::vector<std::vector<float>> framePoints = ApplyWaveEffect(
            basePlanePoints,
            width,
            height,
            time,
            75.0f,  // amplitude
            1.0f,  // frequency
            3.0f    // wave speed
        );

        // Armazenar frame
        preCalculatedFrames.push_back(framePoints);

        // Mostrar progresso
        if (frame % (numFrames / 10) == 0)
        {
            std::cout << "Progress: " << (frame * 100 / numFrames) << "%" << std::endl;
        }
    }

    std::cout << "Pre-calculation completed!" << std::endl;
}

int SendImagePlane(igtl::Socket::Pointer& socket, int frameIndex)
{
    // Verificar se o frame existe
    if (frameIndex >= preCalculatedFrames.size())
    {
        std::cerr << "Frame index out of bounds!" << std::endl;
        return 0;
    }

    // Allocate Image Message Class
    igtl::ImageMessage::Pointer imageMsg;
    imageMsg = igtl::ImageMessage::New();

    imageMsg->SetEndian(2);

    // Set device name
    imageMsg->SetDeviceName("Wave Coordinates");

    // Get current frame points
    const std::vector<std::vector<float>>& framePoints = preCalculatedFrames[frameIndex];

    // Configure image parameters
    int scalarType = igtl::ImageMessage::TYPE_FLOAT32;
    int size[3] = { imageWidth, imageHeight, 1 };    // dimensions
    int subsize[3] = { imageWidth, imageHeight, 1 }; // subvolume size
    int suboffset[3] = { 0, 0, 0 };                  // subvolume offset
    float spacing[3] = { 1.0, 1.0, 1.0 };           // spacing (mm/pixel)
    int svsize[3] = { imageWidth, imageHeight, 1 };  // sub-volume size

    // Set image parameters
    imageMsg->SetDimensions(size);
    imageMsg->SetSpacing(spacing);
    imageMsg->SetScalarType(scalarType);
    imageMsg->SetDeviceName("Wave Coordinates");
    imageMsg->SetNumComponents(3); // 3 channels: X, Y, Z

    // Sub-volume parameters
    imageMsg->SetSubVolume(subsize, suboffset);

    // Allocate scalar array
    imageMsg->AllocateScalars();

    // Get pointer to image data
    float* imageData = (float*)imageMsg->GetScalarPointer();

    // Fill image data
    int totalPixels = imageWidth * imageHeight;

    // Initialize all pixels to zero (for padding)
    for (int i = 0; i < totalPixels * 3; i++)
    {
        imageData[i] = 0.0f;
    }

    // Fill with point coordinates
    for (size_t pointIdx = 0; pointIdx < framePoints.size() && pointIdx < totalPixels; pointIdx++)
    {
        int pixelIdx = pointIdx * 3; // 3 components per pixel

        // Channel 0: X coordinate
        imageData[pixelIdx + 0] = framePoints[pointIdx][0];

        // Channel 1: Y coordinate  
        imageData[pixelIdx + 1] = framePoints[pointIdx][1];

        // Channel 2: Z coordinate
        imageData[pixelIdx + 2] = framePoints[pointIdx][2];
    }

    timeStamp->GetTime();  // Pega tempo atual

    // Atribuir à mensagem
    imageMsg->SetTimeStamp(timeStamp);

    // Pack imageMsg and send via OpenIGTLink socket
    imageMsg->Pack();

    socket->Send(imageMsg->GetPackPointer(), imageMsg->GetPackSize());
    //std::cout << "Wave coordinates frame " << frameIndex << " sent" << std::endl;

    return 1;
}

int LoopFunction() {
    // Configurações da animação
    const int NUM_FRAMES = 60;  // 10 segundos a 30 FPS
    const float FRAME_RATE = 60.0f;
    const int FRAME_INTERVAL_MS = static_cast<int>(1000.0f / FRAME_RATE);

    // Pré-calcular todos os frames
    PreCalculateWaveFrames(PLANE_WIDTH, PLANE_HEIGHT, NUM_FRAMES, FRAME_RATE);

    // Create server socket
    igtl::ServerSocket::Pointer ServerSocket;
    ServerSocket = igtl::ServerSocket::New();
    int r = ServerSocket->CreateServer(18944);
    if (r < 0)
    {
        std::cerr << "Cannot create a server socket" << std::endl;
        exit(0);
    }
    igtl::Socket::Pointer socket;
    // Wait for Connection
    socket = ServerSocket->WaitForConnection(10000); // Wait ten seconds for connection
    if (socket.IsNotNull()) // if client connected
    {
        std::cerr << "Client connected. Starting wave animation..." << std::endl;

        int currentFrame = 0;

        while (!StopFlag)
        {
            auto start = std::chrono::high_resolution_clock::now();

            // Enviar frame atual
            SendImagePlane(socket, currentFrame);

            // Avançar para próximo frame (loop cíclico)
            currentFrame = (currentFrame + 1) % NUM_FRAMES;

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;

            double fps = 1.0 / elapsed.count();
            std::cout << "FPS: " << fps << std::endl;

            //igtl::Sleep(50);
            std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_INTERVAL_MS));
        }
    }
    else
    {
        std::cerr << "Client could not connect." << std::endl;
    }
    // Close connect
    socket->CloseSocket();
    return 1;
}

int main() {
    // Create thread for loopFunction
    std::thread MessageThread(LoopFunction);
    // main function stops to listen for keyboard input. When input is detected,
    // loop in LoopFunction is broken, and the function quickly returns
    std::cout << "Press any key to stop wave animation" << std::endl;
    std::cin.get();
    StopFlag = true;
    MessageThread.join();
    // Send message when all threads are joined
    std::cout << "All threads joined" << std::endl;
    return EXIT_SUCCESS;
}