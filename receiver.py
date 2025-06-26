import util

node = getNode('vtkMRMLVectorVolumeNode1')

data = util.arrayFromVolume(node)

fidNode = slicer.mrmlScene.CreateNodeByClass('vtkMRMLMarkupsFiducialNode')
slicer.mrmlScene.AddNode(fidNode)

data = data[0,...]
H, W, D = data.shape

if fidNode.GetNumberOfControlPoints() < H * W: 
    for i in range(H):
        for j in range(W):
            fidNode.AddControlPoint(data[i,j,:], f'P({i},{j})')
        
else:
    ctrlPoint = 0
    for i in range(H):
        for j in range(W):
            fidNode.SetNthControlPointPosition(ctrlPoint, data[i,j,:])
