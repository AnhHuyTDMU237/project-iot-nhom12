import 'package:flutter/material.dart';

import '../models/event_data.dart';
import '../services/api_service.dart';

class EventsScreen extends StatefulWidget {
  const EventsScreen({super.key});

  @override
  State<EventsScreen> createState() =>
      _EventsScreenState();
}

class _EventsScreenState
    extends State<EventsScreen> {

  late Future<List<EventData>> future;

  @override
  void initState() {
    super.initState();

    future = ApiService.getEvents();
  }

  Future<void> reload() async {
    setState(() {
      future = ApiService.getEvents();
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Cảnh báo',
        ),
      ),

      body: FutureBuilder<List<EventData>>(
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

          final events = snapshot.data ?? [];

          if (events.isEmpty) {
            return const Center(
              child: Text(
                'Chưa có cảnh báo',
              ),
            );
          }

          return RefreshIndicator(
            onRefresh: reload,

            child: ListView.builder(
              padding: const EdgeInsets.all(12),

              itemCount: events.length,

              itemBuilder: (context, index) {
                final event = events[index];

                final isFire =
                    event.event == 'FIRE';

                return Card(
                  child: ListTile(
                    leading: Icon(
                      isFire
                          ? Icons.local_fire_department
                          : Icons.warning,
                      color:
                          isFire
                              ? Colors.red
                              : Colors.orange,
                    ),

                    title: Text(
                      event.event,
                      style: const TextStyle(
                        fontWeight:
                            FontWeight.bold,
                      ),
                    ),

                    subtitle: Text(
                      event.message ??
                          'Không có nội dung',
                    ),

                    trailing: Text(
                      event.severity ??
                          '',
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