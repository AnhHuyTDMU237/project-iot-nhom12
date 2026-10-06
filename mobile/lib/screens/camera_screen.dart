import 'package:flutter/material.dart';
import '../models/camera_image.dart';
import '../services/api_service.dart';

class CameraScreen extends StatefulWidget {
  const CameraScreen({super.key});

  @override
  State<CameraScreen> createState() => _CameraScreenState();
}

class _CameraScreenState extends State<CameraScreen> {
  final ApiService apiService = ApiService();

  List<CameraImage> images = [];

  bool loading = true;
  String? error;

  @override
  void initState() {
    super.initState();
    loadImages();
  }

  Future<void> loadImages() async {
    try {
      setState(() {
        loading = true;
        error = null;
      });

      final result = await apiService.getCameraImages();

      if (!mounted) return;

      setState(() {
        images = result;
        loading = false;
      });
    } catch (e) {
      if (!mounted) return;

      setState(() {
        loading = false;
        error = e.toString();
      });
    }
  }

  String formatDate(DateTime? date) {
    if (date == null) {
      return '--';
    }

    final local = date.toLocal();

    return '${local.day.toString().padLeft(2, '0')}/'
        '${local.month.toString().padLeft(2, '0')}/'
        '${local.year} '
        '${local.hour.toString().padLeft(2, '0')}:'
        '${local.minute.toString().padLeft(2, '0')}:'
        '${local.second.toString().padLeft(2, '0')}';
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Camera',
          style: TextStyle(
            fontWeight: FontWeight.bold,
          ),
        ),
        actions: [
          IconButton(
            onPressed: loadImages,
            icon: const Icon(Icons.refresh),
          ),
        ],
      ),

      body: RefreshIndicator(
        onRefresh: loadImages,

        child: loading
            ? const Center(
                child: CircularProgressIndicator(),
              )

            : error != null
                ? ListView(
                    children: [
                      const SizedBox(height: 150),

                      const Icon(
                        Icons.error_outline,
                        size: 60,
                        color: Colors.redAccent,
                      ),

                      const SizedBox(height: 15),

                      Center(
                        child: Text(
                          'Không thể tải ảnh',
                          style: const TextStyle(
                            fontSize: 18,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                      ),

                      const SizedBox(height: 8),

                      Center(
                        child: Padding(
                          padding: const EdgeInsets.all(20),
                          child: Text(
                            error!,
                            textAlign: TextAlign.center,
                          ),
                        ),
                      ),

                      Center(
                        child: ElevatedButton(
                          onPressed: loadImages,
                          child: const Text(
                            'Thử lại',
                          ),
                        ),
                      ),
                    ],
                  )

                : images.isEmpty
                    ? ListView(
                        children: const [
                          SizedBox(height: 180),

                          Icon(
                            Icons.photo_camera_outlined,
                            size: 70,
                            color: Colors.grey,
                          ),

                          SizedBox(height: 15),

                          Center(
                            child: Text(
                              'Chưa có hình ảnh',
                              style: TextStyle(
                                fontSize: 18,
                                fontWeight: FontWeight.bold,
                              ),
                            ),
                          ),

                          SizedBox(height: 8),

                          Center(
                            child: Text(
                              'Ảnh sẽ xuất hiện khi hệ thống phát hiện FIRE.',
                            ),
                          ),
                        ],
                      )

                    : GridView.builder(
                        padding: const EdgeInsets.all(12),

                        gridDelegate:
                            const SliverGridDelegateWithFixedCrossAxisCount(
                          crossAxisCount: 2,
                          crossAxisSpacing: 12,
                          mainAxisSpacing: 12,
                          childAspectRatio: 0.72,
                        ),

                        itemCount: images.length,

                        itemBuilder: (context, index) {
                          final image = images[index];

                          return CameraCard(
                            image: image,
                            onTap: () {
                              openImageDetail(
                                context,
                                image,
                              );
                            },
                          );
                        },
                      ),
      ),
    );
  }

  void openImageDetail(
    BuildContext context,
    CameraImage image,
  ) {
    showDialog(
      context: context,
      builder: (context) {
        return Dialog(
          backgroundColor: Colors.black,
          insetPadding: const EdgeInsets.all(10),

          child: Stack(
            children: [
              InteractiveViewer(
                minScale: 0.5,
                maxScale: 4.0,

                child: Image.network(
                  image.imageUrl,
                  fit: BoxFit.contain,

                  loadingBuilder:
                      (context, child, loadingProgress) {
                    if (loadingProgress == null) {
                      return child;
                    }

                    return const SizedBox(
                      height: 400,
                      child: Center(
                        child: CircularProgressIndicator(),
                      ),
                    );
                  },

                  errorBuilder:
                      (context, error, stackTrace) {
                    return const SizedBox(
                      height: 400,
                      child: Center(
                        child: Column(
                          mainAxisAlignment:
                              MainAxisAlignment.center,

                          children: [
                            Icon(
                              Icons.broken_image,
                              size: 60,
                              color: Colors.white,
                            ),

                            SizedBox(height: 10),

                            Text(
                              'Không tải được ảnh',
                              style: TextStyle(
                                color: Colors.white,
                              ),
                            ),
                          ],
                        ),
                      ),
                    );
                  },
                ),
              ),

              Positioned(
                top: 8,
                right: 8,

                child: CircleAvatar(
                  backgroundColor:
                      Colors.black.withOpacity(0.7),

                  child: IconButton(
                    onPressed: () {
                      Navigator.pop(context);
                    },

                    icon: const Icon(
                      Icons.close,
                      color: Colors.white,
                    ),
                  ),
                ),
              ),
            ],
          ),
        );
      },
    );
  }
}


// ==========================================================
// CAMERA CARD
// ==========================================================

class CameraCard extends StatelessWidget {
  final CameraImage image;
  final VoidCallback onTap;

  const CameraCard({
    super.key,
    required this.image,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      clipBehavior: Clip.antiAlias,
      elevation: 4,

      child: InkWell(
        onTap: onTap,

        child: Column(
          crossAxisAlignment:
              CrossAxisAlignment.start,

          children: [
            Expanded(
              child: Stack(
                children: [
                  Positioned.fill(
                    child: Image.network(
                      image.imageUrl,
                      fit: BoxFit.cover,

                      errorBuilder:
                          (context, error, stackTrace) {
                        return Container(
                          color: Colors.black12,

                          child: const Center(
                            child: Icon(
                              Icons.broken_image,
                              size: 50,
                              color: Colors.grey,
                            ),
                          ),
                        );
                      },

                      loadingBuilder:
                          (context, child, progress) {
                        if (progress == null) {
                          return child;
                        }

                        return const Center(
                          child:
                              CircularProgressIndicator(),
                        );
                      },
                    ),
                  ),

                  Positioned(
                    top: 8,
                    left: 8,

                    child: Container(
                      padding:
                          const EdgeInsets.symmetric(
                        horizontal: 8,
                        vertical: 4,
                      ),

                      decoration: BoxDecoration(
                        color: Colors.red,
                        borderRadius:
                            BorderRadius.circular(20),
                      ),

                      child: Text(
                        image.event,
                        style: const TextStyle(
                          color: Colors.white,
                          fontWeight: FontWeight.bold,
                          fontSize: 11,
                        ),
                      ),
                    ),
                  ),

                  Positioned(
                    right: 8,
                    bottom: 8,

                    child: Container(
                      padding:
                          const EdgeInsets.all(7),

                      decoration: BoxDecoration(
                        color: Colors.black
                            .withOpacity(0.65),
                        shape: BoxShape.circle,
                      ),

                      child: const Icon(
                        Icons.zoom_in,
                        color: Colors.white,
                        size: 20,
                      ),
                    ),
                  ),
                ],
              ),
            ),

            Padding(
              padding:
                  const EdgeInsets.all(10),

              child: Column(
                crossAxisAlignment:
                    CrossAxisAlignment.start,

                children: [
                  const Text(
                    '🔥 FIRE',
                    style: TextStyle(
                      fontWeight: FontWeight.bold,
                    ),
                  ),

                  const SizedBox(height: 5),

                  Text(
                    _formatDate(
                      image.createdAt,
                    ),

                    style: TextStyle(
                      color: Colors.grey.shade600,
                      fontSize: 11,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  String _formatDate(DateTime? date) {
    if (date == null) {
      return '--';
    }

    final local = date.toLocal();

    return '${local.day.toString().padLeft(2, '0')}/'
        '${local.month.toString().padLeft(2, '0')}/'
        '${local.year} '
        '${local.hour.toString().padLeft(2, '0')}:'
        '${local.minute.toString().padLeft(2, '0')}';
  }
}