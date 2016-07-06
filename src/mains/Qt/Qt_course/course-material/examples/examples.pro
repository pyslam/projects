TEMPLATE=subdirs

exists( activeqt ):SUBDIRS += activeqt
exists( alphaBlending2 ):SUBDIRS += alphaBlending2
exists( anti-aliased ):SUBDIRS += anti-aliased
exists( codec ):SUBDIRS += codec
exists( connect ):SUBDIRS += connect
exists( containers ):SUBDIRS += containers
exists( custom-signals ):SUBDIRS += custom-signals
exists( delayedInit ):SUBDIRS += delayedInit
exists( dynamic-help ):SUBDIRS += dynamic-help
exists( eventFilter ):SUBDIRS += eventFilter
exists( fibonacci ):SUBDIRS += fibonacci
#exists( fileChooser ):SUBDIRS += fileChooser
exists( findTool ):SUBDIRS += findTool
exists( gradients ):SUBDIRS += gradients
exists( helloWorld ):SUBDIRS += helloWorld
exists( house-drawings ):SUBDIRS += house-drawings
exists( inputMasks ):SUBDIRS += inputMasks
exists( kdab ):SUBDIRS += kdab
exists( model-view ):SUBDIRS += model-view
exists( opengl ):SUBDIRS += opengl
exists( paintProgram ):SUBDIRS += paintProgram
exists( pen-with-brush ):SUBDIRS += pen-with-brush
#exists( plugins ):SUBDIRS += plugins
exists( puzzle ):SUBDIRS += puzzle
exists( qdom ):SUBDIRS += qdom
exists( qdom-write ):SUBDIRS += qdom-write
exists( qmatrix ):SUBDIRS += qmatrix
#exists( qsa ):SUBDIRS += qsa
exists( qscrollarea ):SUBDIRS += qscrollarea
#exists( qt-embedded ):SUBDIRS += qt-embedded
exists( qtextstream ):SUBDIRS += qtextstream
exists( qtoolbox ):SUBDIRS += qtoolbox
#exists( qtopia ):SUBDIRS += qtopia
exists( resources ):SUBDIRS += resources
exists( showhide ):SUBDIRS += showhide
exists( signalSlots ):SUBDIRS += signalSlots
exists( sockets ):SUBDIRS += sockets
exists( sql ):SUBDIRS += sql
exists( stopwatch ):SUBDIRS += stopwatch
exists( syntheticEvents ):SUBDIRS += syntheticEvents
exists( transparency ):SUBDIRS += transparency
exists( tux ):SUBDIRS += tux
exists( udpClient ):SUBDIRS += udpClient
exists( udpServer ):SUBDIRS += udpServer
exists( widgetGrouping ):SUBDIRS += widgetGrouping
exists( rect-outline ) :SUBDIRS += rect-outline
exists( widgetPrinting ) :SUBDIRS += widgetPrinting
exists( unit-test ) :SUBDIRS += unit-test

!win32|!exists( $$QTDIR\lib\qaxcontainer.lib ):SUBDIRS -= activeqt

