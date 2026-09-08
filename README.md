# tanks_game
Welcome! This is a personal project game, supporting local single player, local co-op, 

This is a tank-based game

To compile the game for local co-op, run the following commands:
 - cmake -B build/build-client -S cpp -DCMAKE_BUILD_TYPE=Release
 - cmake --build build/build-client --target gs
 - bin/gs.exe

To compile and run tests:
 - cmake -B build/build-tests -S cpp -DCMAKE_BUILD_TYPE=Release -DBUILD_CLIENT=OFF
 - cmake --build build/build-tests --config Release
 - ctest --test-dir build/build-tests --build-config Release --output-on-failure

To compile and train a model, ensure a python venv is active and run these commands:
 - python -m pip install pybind11
 - cmake -B build/build-rl -S cpp -DCMAKE_BUILD_TYPE=Release -DBUILD_CLIENT=OFF -DBUILD_RL=ON
 - cmake --build build/build-rl --target tanks_env_cpp
 - cd python
 - python -m unittest test_rl_config.py
 - python rl-learning.py --opponent-difficulty Easy --learning-rate 1e-3 --net-arch 64,64 --total-timesteps 50000 --model-name tank_ppo_easy

 To compile and train a model for local single player, ensure a python venv is active and run these commands:
  - python -m pip install onnx onnxruntime
  - *Download ONNX Runtime from https://github.com/microsoft/onnxruntime/releases*
  - cmake -B build/build-inference -S cpp -DCMAKE_BUILD_TYPE=Release -DBUILD_RL=ON -DBUILD_ONNX_INFERENCE=ON -DONNXRUNTIME_ROOT_DIR="your onnxruntime path"
  - cmake --build build/build-inference --target gs tanks_env_cpp
  - cd python
  - python rl-learning.py
  - *If on Windows, copy onnxruntime.dll from "your onnxruntime path"\lib\onnxruntime.dll into GameServer/bin*
  - bin\gs.exe --vs-rl python\models\tank_ppo.onnx
  - *To get model and latency metrics, run the following*
    - cmake --build build/build-inference --target bench-inference
    - bin\bench-inference.exe python\models\tank_ppo.onnx
    - cd python
    - python evaluate_win_rate.py --model models/tank_ppo.zip --episodes 300 *--opponent-difficulty [Easy,Medium,Hard] --sample-actions*