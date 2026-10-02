OUT=bv2dedicated
all: DukZeven dko baboNet bv2dedicated copy
bv2dedicated:
	#g++ -w -g -o $(OUT) -D LINUX64 -fPIC -D CONSOLE -IBaboViolent2/Code -IBaboViolent2/inc BaboViolent2/Code/*.cpp -LBaboViolent2/Content -lbaboNet -ldkc -ldkf -ldkgl -ldki -ldko -ldkp -ldks -ldksvar -ldkt -ldkw -lpthread -lsqlite3 -lGLU -lcurl -static-libstdc++ -lm
	g++ -w -g -o $(OUT) -D LINUX64 -fPIC -D CONSOLE -IBaboViolent2/Code -IBaboViolent2/inc BaboViolent2/Code/*.cpp -LBaboViolent2/Content -lbaboNet -ldkc -ldko -ldkt -ldksvar -lpthread -lsqlite3 -lGLU -lcurl -static-libstdc++ -lm
copy:
	mv $(OUT) ./BaboViolent2/Content/

baboNet:
	$(MAKE) -C Engine/babonet/Code

dko:
	$(MAKE) -C Engine/dko/Code

DukZeven:
	$(MAKE) -C Engine/DukZeven/Code console

clean:
	$(MAKE) -C Engine/babonet/Code clean
	$(MAKE) -C Engine/dko/Code clean
	$(MAKE) -C Engine/DukZeven/Code clean
	rm -f $(OUT)
