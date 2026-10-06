import 'dart:convert';

import 'package:http/http.dart' as http;

import '../config/app_config.dart';
import '../models/event_data.dart';
import '../models/sensor_data.dart';

class ApiService {
  static String get baseUrl => AppConfig.apiBaseUrl;

  // =========================================================
  // Lấy sensor mới nhất
  // =========================================================
  static Future<SensorData> getLatestSensor() async {
    final url = Uri.parse(
      '$baseUrl/api/sensors/latest'
      '?device_id=${AppConfig.deviceId}',
    );

    final response = await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Không thể lấy dữ liệu sensor: '
        '${response.statusCode}',
      );
    }

    final data = jsonDecode(response.body);

    if (data['message'] != null) {
      throw Exception(data['message']);
    }

    return SensorData.fromJson(data);
  }

  // =========================================================
  // Lấy lịch sử sensor
  // =========================================================
  static Future<List<SensorData>> getSensorHistory() async {
    final url = Uri.parse(
      '$baseUrl/api/sensors/history'
      '?device_id=${AppConfig.deviceId}',
    );

    final response = await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Không thể lấy lịch sử sensor: '
        '${response.statusCode}',
      );
    }

    final List<dynamic> data = jsonDecode(response.body);

    return data
        .map(
          (item) => SensorData.fromJson(
            item as Map<String, dynamic>,
          ),
        )
        .toList();
  }

  // =========================================================
  // Lấy danh sách event
  // =========================================================
  static Future<List<EventData>> getEvents() async {
    final url = Uri.parse(
      '$baseUrl/api/events'
      '?device_id=${AppConfig.deviceId}',
    );

    final response = await http.get(url);

    if (response.statusCode != 200) {
      throw Exception(
        'Không thể lấy danh sách cảnh báo: '
        '${response.statusCode}',
      );
    }

    final List<dynamic> data = jsonDecode(response.body);

    return data
        .map(
          (item) => EventData.fromJson(
            item as Map<String, dynamic>,
          ),
        )
        .toList();
  }

  // =========================================================
  // Gửi lệnh điều khiển
  // =========================================================
  static Future<void> sendCommand(String command) async {
    final url = Uri.parse(
      '$baseUrl/api/devices/'
      '${AppConfig.deviceId}/command',
    );

    final response = await http.post(
      url,
      headers: {
        'Content-Type': 'application/json',
      },
      body: jsonEncode({
        'command': command,
      }),
    );

    if (response.statusCode < 200 ||
        response.statusCode >= 300) {
      throw Exception(
        'Gửi lệnh thất bại: '
        '${response.statusCode}\n'
        '${response.body}',
      );
    }
  }
}