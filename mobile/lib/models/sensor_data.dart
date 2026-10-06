class SensorData {
  final String deviceId;
  final double? temperature;
  final double? humidity;
  final int? mq2;
  final bool flame;
  final bool pir;
  final DateTime? createdAt;

  SensorData({
    required this.deviceId,
    this.temperature,
    this.humidity,
    this.mq2,
    required this.flame,
    required this.pir,
    this.createdAt,
  });

  factory SensorData.fromJson(Map<String, dynamic> json) {
    return SensorData(
      deviceId: json['device_id'] ?? '',
      temperature: (json['temperature'] as num?)?.toDouble(),
      humidity: (json['humidity'] as num?)?.toDouble(),
      mq2: (json['mq2'] as num?)?.toInt(),
      flame: json['flame'] ?? false,
      pir: json['pir'] ?? false,
      createdAt: json['created_at'] != null
          ? DateTime.tryParse(json['created_at'].toString())
          : null,
    );
  }
}