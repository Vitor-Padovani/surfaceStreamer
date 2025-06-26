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
#include "igtlPolyDataMessage.h"

const double M_PI = 3.1415926535;

const int PLANE_WIDTH = 100;
const int PLANE_HEIGHT = 100;

std::atomic_bool StopFlag = false;

// Vectors para armazenar todos os frames pré-calculados
std::vector<std::vector<std::vector<float>>> preCalculatedFrames;
std::vector<igtl::PolyDataPointArray::Pointer> preCalculatedPointArrays;

// Pre-calculated lines (grid topology doesn't change)
igtl::PolyDataCellArray::Pointer preCalculatedLines;

// Criar e configurar timestamp
igtl::TimeStamp::Pointer timeStamp = igtl::TimeStamp::New();

// Função que gera um plano de pontos
std::vector<std::vector<float>> GeneratePlanePoints(
    int width, int height, float spacing = 10.0f, float z_level = 0.0f, float offset = -100.0f)
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

// Função que pré-calcula as linhas da grade (chamada apenas uma vez)
void PreCalculateLines(int width, int height) {
    std::cout << "Pre-calculating grid lines..." << std::endl;

    preCalculatedLines = igtl::PolyDataCellArray::New();

    // Add horizontal lines
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width - 1; j++) {
            igtlUint32 lineData[2] = {
                static_cast<igtlUint32>(i * width + j),
                static_cast<igtlUint32>(i * width + j + 1)
            };
            preCalculatedLines->AddCell(2, lineData);
        }
    }

    // Add vertical lines
    for (int i = 0; i < height - 1; i++) {
        for (int j = 0; j < width; j++) {
            igtlUint32 lineData[2] = {
                static_cast<igtlUint32>(i * width + j),
                static_cast<igtlUint32>((i + 1) * width + j)
            };
            preCalculatedLines->AddCell(2, lineData);
        }
    }

    std::cout << "Grid lines pre-calculation completed!" << std::endl;
}

// Função que pré-calcula todos os frames do wave effect
void PreCalculateWaveFrames(int width, int height, int numFrames, float frameRate = 30.0f)
{
    std::cout << "Pre-calculating " << numFrames << " wave frames..." << std::endl;

    // Limpar frames anteriores
    preCalculatedFrames.clear();
    preCalculatedPointArrays.clear();
    preCalculatedFrames.reserve(numFrames);
    preCalculatedPointArrays.reserve(numFrames);

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
            60.0f,  // amplitude
            0.3f,  // frequency
            3.0f    // wave speed
        );

        // Criar pointArray pré-calculado para este frame
        igtl::PolyDataPointArray::Pointer pointArray = igtl::PolyDataPointArray::New();

        int numPoints = framePoints.size();
        for (int i = 0; i < numPoints; i++)
        {
            igtlFloat32 point[3] = {
                static_cast<igtlFloat32>(framePoints[i][0]),
                static_cast<igtlFloat32>(framePoints[i][1]),
                static_cast<igtlFloat32>(framePoints[i][2])
            };
            pointArray->AddPoint(point);
        }

        // Armazenar ambos
        preCalculatedFrames.push_back(framePoints);
        preCalculatedPointArrays.push_back(pointArray);

        // Mostrar progresso
        if (frame % (numFrames / 10) == 0)
        {
            std::cout << "Progress: " << (frame * 100 / numFrames) << "%" << std::endl;
        }
    }

    std::cout << "Pre-calculation completed!" << std::endl;
}

int SendPolyDataPlane(igtl::Socket::Pointer& socket, int frameIndex)
{
    // Verificar se o frame existe
    if (frameIndex >= preCalculatedPointArrays.size())
    {
        std::cerr << "Frame index out of bounds!" << std::endl;
        return 0;
    }

    // Allocate PolyData Message Class
    igtl::PolyDataMessage::Pointer polyDataMsg;
    polyDataMsg = igtl::PolyDataMessage::New();

    // Set device name
    polyDataMsg->SetDeviceName("Wave Plane");

    // Usar pointArray pré-calculado (muito mais rápido!)
    polyDataMsg->SetPoints(preCalculatedPointArrays[frameIndex]);

    // Usar lines pré-calculadas (muito mais rápido!)
    polyDataMsg->SetLines(preCalculatedLines);

    timeStamp->GetTime();  // Pega tempo atual

    // Atribuir à mensagem
    polyDataMsg->SetTimeStamp(timeStamp);

    // Pack polyDataMsg and send via OpenIGTLink socket
    polyDataMsg->Pack();

    socket->Send(polyDataMsg->GetPackPointer(), polyDataMsg->GetPackSize());
    //std::cout << "Wave plane frame " << frameIndex << " sent" << std::endl;

    return 1;
}

int SendPolyData(igtl::Socket::Pointer& socket)
{
    // Allocate Status Message Class
    igtl::PolyDataMessage::Pointer polyDataMsg;
    polyDataMsg = igtl::PolyDataMessage::New();
    // NOTE: the server should send a message with the same device name
    // as the received query message. 
    polyDataMsg->SetDeviceName("Fake Shapes");
    // Define points connected by line
    igtlFloat32 pointsData[10][3];
    for (int i = 0; i < 10; i++) {
        pointsData[i][0] = i * 10;
        pointsData[i][1] = i * 10;
        pointsData[i][2] = i * 10;
    }
    // Create point array
    igtl::PolyDataPointArray::Pointer pointArray;
    pointArray = igtl::PolyDataPointArray::New();
    // Add point data to point array
    for (unsigned int i = 0; i < 10; i++)
    {
        pointArray->AddPoint(pointsData[i]);
    }
    // Add pointArray to PolyDataMessage
    polyDataMsg->SetPoints(pointArray);
    // Add lines
    static igtlUint32 lineData[10] = { 0,1,2,3,4,5,6,7,8,9 }; // equivalent to SetId with vtk's PolyLine
    // Create PolyDataCellArray and add lines
    igtl::PolyDataCellArray::Pointer cellArray;
    cellArray = igtl::PolyDataCellArray::New();
    cellArray->AddCell(10, lineData);
    // Add PolyDataCellArray to PolyDataMessage
    polyDataMsg->SetLines(cellArray);
    // Pack polyDataMsg and send via OpenIGTLink socket
    polyDataMsg->Pack();
    socket->Send(polyDataMsg->GetPackPointer(), polyDataMsg->GetPackSize());
    std::cout << "Message sent" << std::endl;
    return 1;
}

int LoopFunction() {
    // Configurações da animação
    const int NUM_FRAMES = 300;  // 10 segundos a 30 FPS
    const float FRAME_RATE = 60.0f;
    const int FRAME_INTERVAL_MS = static_cast<int>(1000.0f / FRAME_RATE);

    // Pré-calcular as linhas da grade (apenas uma vez)
    PreCalculateLines(PLANE_WIDTH, PLANE_HEIGHT);

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
            SendPolyDataPlane(socket, currentFrame);

            // Avançar para próximo frame (loop cíclico)
            currentFrame = (currentFrame + 1) % NUM_FRAMES;

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;

            double fps = 1.0 / elapsed.count();
            std::cout << "FPS: " << fps << std::endl;

            //igtl::Sleep(50);
            //std::this_thread::sleep_for(std::chrono::milliseconds(50));
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