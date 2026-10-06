class EventData {
  final int id;
  final String deviceId;
  final String event;
  final String? severity;
  final String? message;
  final DateTime? createdAt;

  EventData({
    required this.id,
    required this.deviceId,
    required this.event,
    this.severity,
    this.message,
    this.createdAt,
  });

  factory EventData.fromJson(Map<String, dynamic> json) {
    return EventData(
      id: json['id'] ?? 0,
      deviceId: json['device_id'] ?? '',
      event: json['event'] ?? '',
      severity: json['severity'],
      message: json['message'],
      createdAt: json['created_at'] != null
          ? DateTime.tryParse(json['created_at'].toString())
          : null,
    );
  }
}