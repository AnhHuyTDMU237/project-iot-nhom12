import '../config/app_config.dart';

class CameraImage {
  final int id;
  final String deviceId;
  final String event;
  final String imagePath;
  final String imageUrl;
  final DateTime? createdAt;

  CameraImage({
    required this.id,
    required this.deviceId,
    required this.event,
    required this.imagePath,
    required this.imageUrl,
    this.createdAt,
  });

  factory CameraImage.fromJson(Map<String, dynamic> json) {
    final rawImageUrl =
        (json['image_url'] ?? '').toString();

    String fullImageUrl;

    if (rawImageUrl.startsWith('http://') ||
        rawImageUrl.startsWith('https://')) {
      fullImageUrl = rawImageUrl;
    } else {
      fullImageUrl =
          '${AppConfig.apiBaseUrl}$rawImageUrl';
    }

    return CameraImage(
      id: json['id'] ?? 0,

      deviceId:
          (json['device_id'] ?? '').toString(),

      event:
          (json['event'] ?? '').toString(),

      imagePath:
          (json['image_path'] ?? '').toString(),

      imageUrl:
          fullImageUrl,

      createdAt:
          json['created_at'] != null
              ? DateTime.tryParse(
                  json['created_at'].toString(),
                )
              : null,
    );
  }
}