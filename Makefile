include ../../env.mk

INC = -I../../dv-user-driver/include -I../common/include -I/usr/include/agg2 `freetype-config --cflags`
LIB = -L../../dv-user-driver -ldmpdv -L../common/lib -ldv700_util -lagg -lfreetype

CFLAGS = -pthread -std=c++11 $(OPT) -Wall -Werror -c $(INC)
LFLAGS = -pthread $(LIB)

DEPS = YOLOv3_gen.h YOLOv3_param.h LaneDetection_gen.h LaneDetection_param.h
OBJS = YOLOv3Lane.o YOLOv3_gen.o YOLOv3_post.o LaneDetection_gen.o LaneDetection_post.o Contours.o
TGT  = ../bin/YOLOv3Lane

DEPS2 = LaneDetection_gen.h LaneDetection_param.h
OBJS2 = LaneDetection_gen.o Contours.o LaneDetection.o
TGT2  = ../bin/LaneDetection

all: $(TGT) $(TGT2)

$(TGT): $(OBJS)
	$(GPP) $^ -o $@ $(LFLAGS)

$(TGT2): $(OBJS2)
	$(GPP) $^ -o $@ $(LFLAGS)

%.o: %.cpp $(DEPS) $(DEFS2)
	$(GPP) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o $(TGT)
