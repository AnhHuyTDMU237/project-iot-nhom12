import 'dart:async';

import 'package:flutter/material.dart';

import '../models/sensor_data.dart';
import '../services/api_service.dart';
import 'control_screen.dart';
import 'events_screen.dart';
import 'history_screen.dart';
import 'camera_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  SensorData? sensor;

  bool loading = true;
  String? error;

  Timer? timer;

  @override
  void initState() {
    super.initState();

    loadSensor();

    timer = Timer.periodic(
      const Duration(seconds: 3),
      (_) => loadSensor(),
    );
  }

  @override
  void dispose() {
    timer?.cancel();
    super.dispose();
  }

  Future<void> loadSensor() async {
    try {
      final result =
          await ApiService.getLatestSensor();

      if (!mounted) return;

      setState(() {
        sensor = result;
        loading = false;
        error = null;
      });
    } catch (e) {
      if (!mounted) return;

      setState(() {
        loading = false;
        error = e.toString();
      });
    }
  }

  String get systemState {
    if (sensor == null) {
      return 'UNKNOWN';
    }

    if (sensor!.flame) {
      return 'FIRE ALERT';
    }

    if ((sensor!.mq2 ?? 0) >= 300) {
      return 'MQ2 ALERT';
    }

    return 'NORMAL';
  }

  Color get stateColor {
    switch (systemState) {
      case 'FIRE ALERT':
        return Colors.red;
      case 'MQ2 ALERT':
        return Colors.orange;
      case 'NORMAL':
        return Colors.green;
      default:
        return Colors.grey;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'FIRE GAS SYSTEM',
        ),
        centerTitle: true,
      ),

      body: RefreshIndicator(
        onRefresh: loadSensor,

        child: ListView(
          padding: const EdgeInsets.all(16),
          children: [

            // =============================================
            // SYSTEM STATUS
            // =============================================
            Card(
              child: Padding(
                padding: const EdgeInsets.all(20),

                child: Column(
                  children: [

                    const Text(
                      'SYSTEM STATUS',
                      style: TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.bold,
                      ),
                    ),

                    const SizedBox(height: 10),

                    Icon(
                      systemState == 'NORMAL'
                          ? Icons.check_circle
                          : Icons.warning,
                      size: 55,
                      color: stateColor,
                    ),

                    const SizedBox(height: 8),

                    Text(
                      systemState,
                      style: TextStyle(
                        fontSize: 24,
                        fontWeight: FontWeight.bold,
                        color: stateColor,
                      ),
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 12),

            if (loading && sensor == null)
              const Center(
                child: CircularProgressIndicator(),
              ),

            if (error != null && sensor == null)
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Text(
                    'Lỗi kết nối:\n$error',
                    style: const TextStyle(
                      color: Colors.red,
                    ),
                  ),
                ),
              ),

            if (sensor != null) ...[

              // ===========================================
              // TEMPERATURE / HUMIDITY
              // ===========================================

              Row(
                children: [

                  Expanded(
                    child: _SensorCard(
                      icon: Icons.thermostat,
                      title: 'Temperature',
                      value:
                          '${sensor!.temperature?.toStringAsFixed(1) ?? '--'} °C',
                    ),
                  ),

                  const SizedBox(width: 10),

                  Expanded(
                    child: _SensorCard(
                      icon: Icons.water_drop,
                      title: 'Humidity',
                      value:
                          '${sensor!.humidity?.toStringAsFixed(1) ?? '--'} %',
                    ),
                  ),
                ],
              ),

              const SizedBox(height: 10),

              // ===========================================
              // MQ2
              // ===========================================

              _SensorCard(
                icon: Icons.air,
                title: 'MQ2 GAS / SMOKE',
                value: '${sensor!.mq2 ?? '--'}',
              ),

              const SizedBox(height: 10),

              Row(
                children: [

                  Expanded(
                    child: _SensorCard(
                      icon: Icons.local_fire_department,
                      title: 'FLAME',
                      value:
                          sensor!.flame ? 'ON' : 'OFF',
                      valueColor:
                          sensor!.flame
                              ? Colors.red
                              : Colors.green,
                    ),
                  ),

                  const SizedBox(width: 10),

                  Expanded(
                    child: _SensorCard(
                      icon: Icons.person,
                      title: 'PIR',
                      value:
                          sensor!.pir ? 'ON' : 'OFF',
                      valueColor:
                          sensor!.pir
                              ? Colors.orange
                              : Colors.grey,
                    ),
                  ),
                ],
              ),

              const SizedBox(height: 20),

              // ===========================================
              // BUTTONS
              // ===========================================

              ElevatedButton.icon(
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (_) =>
                          const ControlScreen(),
                    ),
                  );
                },
                icon: const Icon(
                  Icons.settings_remote,
                ),
                label: const Text(
                  'ĐIỀU KHIỂN THIẾT BỊ',
                ),
              ),

              const SizedBox(height: 10),

              ElevatedButton.icon(
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (_) =>
                          const HistoryScreen(),
                    ),
                  );
                },
                icon: const Icon(
                  Icons.history,
                ),
                label: const Text(
                  'LỊCH SỬ SENSOR',
                ),
              ),

              const SizedBox(height: 10),

              ElevatedButton.icon(
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (_) =>
                          const EventsScreen(),
                    ),
                  );
                },
                icon: const Icon(
                  Icons.warning,
                ),
                label: const Text(
                  'LỊCH SỬ CẢNH BÁO',
                ),
              ),

              const SizedBox(height: 10),

              // ===========================================
              // CAMERA
              // ===========================================

              ElevatedButton.icon(
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (_) =>
                          const CameraScreen(),
                    ),
                  );
                },
                icon: const Icon(
                  Icons.photo_camera,
                ),
                label: const Text(
                  'XEM ẢNH CAMERA',
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }
}


// =========================================================
// SENSOR CARD
// =========================================================

class _SensorCard extends StatelessWidget {
  final IconData icon;
  final String title;
  final String value;
  final Color? valueColor;

  const _SensorCard({
    required this.icon,
    required this.title,
    required this.value,
    this.valueColor,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),

        child: Column(
          children: [

            Icon(
              icon,
              size: 32,
            ),

            const SizedBox(height: 8),

            Text(
              title,
              textAlign: TextAlign.center,
              style: const TextStyle(
                fontSize: 13,
              ),
            ),

            const SizedBox(height: 5),

            Text(
              value,
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: valueColor,
              ),
            ),
          ],
        ),
      ),
    );
  }
}