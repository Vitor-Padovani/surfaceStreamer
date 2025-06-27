import slicer
import numpy as np
import vtk

# Assumindo que util é um módulo customizado - se não for, use slicer.util
import util

def myfunc():

    node = getNode('vtkMRMLVectorVolumeNode1')

    dataRaw = util.arrayFromVolume(node)

    # Debug: verificar dimensões dos dados
    print(f"Data shape: {dataRaw.shape}")
    print(f"Data type: {dataRaw.dtype}")

    # Extrair slice (assumindo que queremos o primeiro slice Z)
    data = dataRaw[0, ...]
    print(f"Slice shape: {data.shape}")

    # Criar tabela para plotar
    tableNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLTableNode")
    table = tableNode.GetTable()

    # Arrays para X, Y, Z
    arrX = vtk.vtkFloatArray()
    arrX.SetName("x")
    table.AddColumn(arrX)

    arrY = vtk.vtkFloatArray()
    arrY.SetName("y")
    table.AddColumn(arrY)

    arrZ = vtk.vtkFloatArray()
    arrZ.SetName("z")
    table.AddColumn(arrZ)

    # Verificar se temos dados suficientes
    if data.shape[0] > 20 and data.shape[1] > 0:
        # Pegar uma linha específica (linha 20) para visualizar o perfil da onda
        row_index = 20
        numPoints = data.shape[1]  # número de colunas (pontos na linha)
        
        table.SetNumberOfRows(numPoints)
        
        for i in range(numPoints):
            # Coordenada X do ponto (canal 0)
            x_coord = float(data[row_index, i, 0])
            # Coordenada Y do ponto (canal 1) 
            y_coord = float(data[row_index, i, 1])
            # Coordenada Z do ponto (canal 2) - esta deve mostrar a onda
            z_coord = float(data[row_index, i, 2])
            
            table.SetValue(i, 0, x_coord)  # X real do ponto
            table.SetValue(i, 1, y_coord)  # Y real do ponto  
            table.SetValue(i, 2, z_coord)  # Z real do ponto (altura da onda)
        
        # Criar série de plot para X vs Z (perfil da onda)
        plotSeriesNode1 = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLPlotSeriesNode", "Wave Profile X-Z")
        plotSeriesNode1.SetAndObserveTableNodeID(tableNode.GetID())
        plotSeriesNode1.SetXColumnName("x")
        plotSeriesNode1.SetYColumnName("z")  # Z mostra a altura da onda
        plotSeriesNode1.SetPlotType(slicer.vtkMRMLPlotSeriesNode.PlotTypeScatter)
        plotSeriesNode1.SetLineStyle(slicer.vtkMRMLPlotSeriesNode.LineStyleSolid)
        plotSeriesNode1.SetMarkerStyle(slicer.vtkMRMLPlotSeriesNode.MarkerStyleCircle)
        plotSeriesNode1.SetUniqueColor()
        
        # Criar série de plot para Y vs Z (outro perfil da onda)
        plotSeriesNode2 = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLPlotSeriesNode", "Wave Profile Y-Z")
        plotSeriesNode2.SetAndObserveTableNodeID(tableNode.GetID())
        plotSeriesNode2.SetXColumnName("y")
        plotSeriesNode2.SetYColumnName("z")  # Z mostra a altura da onda
        plotSeriesNode2.SetPlotType(slicer.vtkMRMLPlotSeriesNode.PlotTypeScatter)
        plotSeriesNode2.SetLineStyle(slicer.vtkMRMLPlotSeriesNode.LineStyleSolid)
        plotSeriesNode2.SetMarkerStyle(slicer.vtkMRMLPlotSeriesNode.MarkerStyleSquare)
        plotSeriesNode2.SetUniqueColor()
        
        # Criar gráfico
        plotChartNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLPlotChartNode")
        plotChartNode.AddAndObservePlotSeriesNodeID(plotSeriesNode1.GetID())
        plotChartNode.AddAndObservePlotSeriesNodeID(plotSeriesNode2.GetID())
        plotChartNode.SetTitle(f'Wave Profile - Row {row_index}')
        plotChartNode.SetXAxisTitle('Position')
        plotChartNode.SetYAxisTitle('Wave Height (Z)')
        
        # Configurar layout com plot
        layoutManager = slicer.app.layoutManager()
        layoutWithPlot = slicer.modules.plots.logic().GetLayoutWithPlot(layoutManager.layout)
        layoutManager.setLayout(layoutWithPlot)
        
        # Selecionar gráfico na view
        plotWidget = layoutManager.plotWidget(0)
        plotViewNode = plotWidget.mrmlPlotViewNode()
        plotViewNode.SetPlotChartNodeID(plotChartNode.GetID())
        
        print(f"Plot created for row {row_index} with {numPoints} points")
        print(f"X range: {min([table.GetValue(i, 0).ToFloat() for i in range(numPoints)])} to {max([table.GetValue(i, 0).ToFloat() for i in range(numPoints)])}")
        print(f"Z range: {min([table.GetValue(i, 2).ToFloat() for i in range(numPoints)])} to {max([table.GetValue(i, 2).ToFloat() for i in range(numPoints)])}")

    else:
        print("Insufficient data or incorrect dimensions")
        print(f"Data shape: {data.shape}")

myfunc()