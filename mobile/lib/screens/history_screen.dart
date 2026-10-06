import 'package:flutter/material.dart';

import '../models/sensor_data.dart';
import '../services/api_service.dart';

class HistoryScreen extends StatefulWidget {
  const HistoryScreen({super.key});

  @override
  State<HistoryScreen> createState() =>
      _HistoryScreenState();
}

class _HistoryScreenState
    extends State<HistoryScreen> {

  late Future<List<SensorData>> future;

  @override
  void initState() {
    super.initState();
    future = ApiService.getSensorHistory();
  }

  Future<void> reload() async {
    setState(() {
      future = ApiService.getSensorHistory();
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Lịch sử sensor',
        ),
      ),

      body: FutureBuilder<List<SensorData>>(
        future: future,

        builder: (context, snapshot) {

          if (snapshot.connectionState ==
              ConnectionState.waiting) {
            return const Center(
              child: CircularProgressIndicator(),
            );
          }

          if (snapshot.hasError) {
            return Center(
              child: Text(
                'Lỗi: ${snapshot.error}',
              ),
            );
          }

          final data = snapshot.data ?? [];

          if (data.isEmpty) {
            return const Center(
              child: Text(
                'Chưa có dữ liệu',
              ),
            );
          }

          return RefreshIndicator(
            onRefresh: reload,

            child: ListView.builder(
              padding: const EdgeInsets.all(12),

              itemCount: data.length,

              itemBuilder: (context, index) {
                final item = data[index];

                return Card(
                  child: ListTile(
                    leading: const Icon(
                      Icons.sensors,
                    ),

                    title: Text(
                      'MQ2: ${item.mq2 ?? '--'}',
                    ),

                    subtitle: Text(
                      'Temp: '
                      '${item.temperature?.toStringAsFixed(1) ?? '--'} °C\n'
                      'Humidity: '
                      '${item.humidity?.toStringAsFixed(1) ?? '--'} %\n'
                      'Flame: '
                      '${item.flame ? 'ON' : 'OFF'} | '
                      'PIR: '
                      '${item.pir ? 'ON' : 'OFF'}',
                    ),
                  ),
                );
              },
            ),
          );
        },
      ),
    );
  }
}