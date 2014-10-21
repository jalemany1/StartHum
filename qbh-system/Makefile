PLAY_LIBRARY := -Llib/portaudio -lportaudio
AUBIO_LIBRARY := -Llib/aubio -laubio
ESSENTIA_LIBRARY := -Llib/essentia -lvamp_essentia
XML_LIBRARY := -Llib/tinyxml -ltinyxml2

all:
	g++ -o build/matching src/similarity_retrieval/matching.cpp $(XML_LIBRARY) -w
	g++ -o build/proof src/similarity_retrieval/proof.cpp $(XML_LIBRARY) -w
	g++ -o build/play src/music_player/play.cpp $(PLAY_LIBRARY) -w
	g++ -o build/melody src/feature_extraction/melody/melody_extraction.cpp $(AUBIO_LIBRARY) -w
	g++ -o build/predominant_melody src/feature_extraction/predominant_melody/predominant_melody_extraction.cpp $(ESSENTIA_LIBRARY) -w
	javac src/connection/ServidorFichero.java src/connection/WorkerRunnable.java

db:	*
	$(shell for i in {101..150}; do for file in media/songs/$${i#1}/*; do build/predominant_melody $$file db/$${file:12:4}; done; done)

clean:
	rm build/matching
	rm build/play
	rm build/melody
	rm build/predominant_melody
	$(shell for i in {101..150}; do rm db/$${i#1}/0; done)