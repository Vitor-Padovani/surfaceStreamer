import slicer
import numpy as np

import util

node = getNode('vtkMRMLVectorVolumeNode1')

dataRaw = util.arrayFromVolume(node)

fidNode = slicer.mrmlScene.CreateNodeByClass('vtkMRMLMarkupsFiducialNode')
slicer.mrmlScene.AddNode(fidNode)

data = dataRaw[0,...]

tableNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLTableNode")
table = tableNode.GetTable()


arrX = vtk.vtkFloatArray()
arrX.SetName("x")
table.AddColumn(arrX)

arrY1 = vtk.vtkFloatArray()
arrY1.SetName("cos")
table.AddColumn(arrY1)

# Fill in the table with some example values

import math
numPoints = data.shape[1]
table.SetNumberOfRows(numPoints)
for i in range(numPoints):
  table.SetValue(i, 0, i )
  table.SetValue(i, 1, vtk.vtkVariant(float(data[50,i,1])))
  
# Create two plot series nodes

plotSeriesNode1 = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLPlotSeriesNode", "My first")
plotSeriesNode1.SetAndObserveTableNodeID(tableNode.GetID())
plotSeriesNode1.SetXColumnName("x")
plotSeriesNode1.SetYColumnName("cos")
plotSeriesNode1.SetPlotType(slicer.vtkMRMLPlotSeriesNode.PlotTypeScatter)
plotSeriesNode1.SetLineStyle(slicer.vtkMRMLPlotSeriesNode.LineStyleNone)
plotSeriesNode1.SetMarkerStyle(slicer.vtkMRMLPlotSeriesNode.MarkerStyleSquare)
plotSeriesNode1.SetUniqueColor()



# Create plot chart node

plotChartNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLPlotChartNode")
plotChartNode.AddAndObservePlotSeriesNodeID(plotSeriesNode1.GetID())
plotChartNode.SetTitle('A simple plot with 2 curves')
plotChartNode.SetXAxisTitle('Something in x')
plotChartNode.SetYAxisTitle('Something in y')

# Switch to a layout that contains a plot view to create a plot widget

layoutManager = slicer.app.layoutManager()
layoutWithPlot = slicer.modules.plots.logic().GetLayoutWithPlot(layoutManager.layout)
layoutManager.setLayout(layoutWithPlot)

# Select chart in plot view

plotWidget = layoutManager.plotWidget(0)
plotViewNode = plotWidget.mrmlPlotViewNode()
plotViewNode.SetPlotChartNodeID(plotChartNode.GetID())