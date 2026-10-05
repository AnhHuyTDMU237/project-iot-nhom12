from datetime import datetime

from sqlalchemy import (
    Column,
    Integer,
    String,
    Float,
    Boolean,
    DateTime,
    Text
)

from database import Base


class Device(Base):
    __tablename__ = "devices"

    id = Column(Integer, primary_key=True, index=True)

    device_id = Column(
        String(100),
        unique=True,
        nullable=False,
        index=True
    )

    name = Column(String(100), nullable=True)

    status = Column(
        String(30),
        default="OFFLINE"
    )

    last_seen = Column(
        DateTime,
        nullable=True
    )


class SensorData(Base):
    __tablename__ = "sensor_data"

    id = Column(Integer, primary_key=True, index=True)

    device_id = Column(
        String(100),
        nullable=False,
        index=True
    )

    temperature = Column(Float, nullable=True)

    humidity = Column(Float, nullable=True)

    mq2 = Column(Integer, nullable=True)

    flame = Column(Boolean, default=False)

    pir = Column(Boolean, default=False)

    created_at = Column(
        DateTime,
        default=datetime.utcnow
    )


class Event(Base):
    __tablename__ = "events"

    id = Column(Integer, primary_key=True, index=True)

    device_id = Column(
        String(100),
        nullable=False,
        index=True
    )

    event = Column(String(50), nullable=False)

    severity = Column(String(30), nullable=True)

    message = Column(Text, nullable=True)

    created_at = Column(
        DateTime,
        default=datetime.utcnow
    )


class CameraImage(Base):
    __tablename__ = "camera_images"

    id = Column(Integer, primary_key=True, index=True)

    device_id = Column(
        String(100),
        nullable=False,
        index=True
    )

    event = Column(String(50), nullable=True)

    image_path = Column(Text, nullable=False)

    created_at = Column(
        DateTime,
        default=datetime.utcnow
    )