for mesh in ../meshes/data_benching/bigguyT.obj; do  
	echo $mesh
	for CPU in 1 2 4 8 16 36; do
		export OMP_NUM_THREADS=$CPU
		echo "$CPU CPUs"
		for D in {1..6}; do
			../build/bench_loop $mesh $D 0 50;
		done
	done
done

