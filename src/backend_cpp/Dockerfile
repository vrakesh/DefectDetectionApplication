# DDA Backend C++ Dockerfile
# Multi-stage build integrating EdgeML SDK with Triton inference

ARG OS=22.04

# ============================================
# Stage 1: Build EdgeML SDK
# ============================================
FROM public.ecr.aws/ubuntu/ubuntu:${OS} AS edgeml-builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies for EdgeML SDK
RUN apt-get update && apt-get install -y \
    build-essential software-properties-common lsb-release git curl wget \
    libcurl4 libcurl4-openssl-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    libsqlite3-dev gnupg pkg-config gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa \
    gstreamer1.0-gl gstreamer1.0-gtk3 libgstrtspserver-1.0-dev libgirepository1.0-dev \
    libudev-dev libssl-dev --fix-missing -y && \
    rm -rf /var/lib/apt/lists/*

# Install gcc-11
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
    apt-get update && \
    apt install gcc-11 g++-11 -y && \
    rm /usr/bin/gcc && \
    rm /usr/bin/g++ && \
    ln -s /usr/bin/gcc-11 /usr/bin/gcc && \
    ln -s /usr/bin/g++-11 /usr/bin/g++

# Install cmake
RUN apt-get update && apt-get install -y cmake ninja-build

# Install meson for aravis
RUN apt-get install -y python3 python3-pip && \
    pip3 install meson

# Install aravis SDK
RUN mkdir -p /dependencies && \
    cd /dependencies && \
    git clone https://github.com/AravisProject/aravis.git && \
    cd aravis && \
    git checkout 0.8.31 && \
    meson setup build -Dtests=false && \
    cd build && \
    ninja && \
    ninja install && \
    ldconfig

# Install boost
RUN apt-get install -y libboost-all-dev

# Copy EdgeML SDK source
COPY ../edgemlsdk/src/include /opt/edgeml/include
COPY ../edgemlsdk/src/src /opt/edgeml/src

# ============================================
# Stage 2: Build C++ Backend
# ============================================
FROM public.ecr.aws/ubuntu/ubuntu:${OS} AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config git wget software-properties-common \
    libsqlite3-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
    gstreamer1.0-libav libssl-dev zlib1g-dev libb64-dev \
    meson ninja-build libxml2-dev libglib2.0-dev libusb-1.0-0-dev \
    gobject-introspection libgirepository1.0-dev libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Update GCC to version 11+
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
    apt-get update && \
    apt-get install -y gcc-11 g++-11 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100

# Build and install Aravis from source (for GigE Vision camera support)
WORKDIR /tmp
RUN git clone https://github.com/AravisProject/aravis.git && \
    cd aravis && \
    git checkout 0.8.31 && \
    meson setup build && \
    cd build && \
    ninja && \
    ninja install && \
    ldconfig

# Copy EdgeML SDK headers for compilation
COPY edgemlsdk /tmp/edgemlsdk

# Install EdgeML SDK packages following install_edgemlsdk.sh
RUN if [ -d /tmp/edgemlsdk/debs ]; then \
        echo "Installing EdgeML SDK packages..." && \
        # Install AWS packages
        dpkg -i /tmp/edgemlsdk/debs/aws-c-iot.deb || true && \
        dpkg -i /tmp/edgemlsdk/debs/aws-crt-cpp.deb || true && \
        dpkg -i /tmp/edgemlsdk/debs/aws-iot-device-sdk-cpp-v2.deb || true && \
        dpkg -i /tmp/edgemlsdk/debs/aws-sdk-cpp.deb || true && \
        # Install supporting libraries (skip GStreamer/libstdc++ - use system versions)
        dpkg -i /tmp/edgemlsdk/debs/liborc-0.4-0.deb || true && \
        dpkg -i /tmp/edgemlsdk/debs/openssl.deb || true && \
        # Install Triton and Panorama packages
        dpkg -i /tmp/edgemlsdk/debs/triton-core.deb || true && \
        dpkg -i /tmp/edgemlsdk/debs/triton-python-backend.deb || true && \
        dpkg -i /tmp/edgemlsdk/debs/PanoramaSDK.deb || true && \
        apt-get install -f -y && \
        ldconfig; \
    fi

# Extract Triton installation files to /opt/
RUN if [ -f /tmp/edgemlsdk/tars/triton_installation_files.tar.gz ]; then \
        echo "Extracting Triton installation files..." && \
        tar xvfz /tmp/edgemlsdk/tars/triton_installation_files.tar.gz -C /opt/ && \
        ls -la /opt/tritonserver/ 2>/dev/null || echo "Triton files extracted"; \
    fi

# Patch triton stubs with real libtritonserver.so from installation
RUN STUB_LOCATION=$(find / -name libtritonserver.so -path "*/tritonserver/install/lib/stubs/*" 2>/dev/null | head -1) && \
    if [ -n "$STUB_LOCATION" ] && [ -f /opt/tritonserver/lib/libtritonserver.so ]; then \
        echo "Patching Triton stub with real library: $STUB_LOCATION" && \
        cp /opt/tritonserver/lib/libtritonserver.so "$STUB_LOCATION" && \
        cp /opt/tritonserver/lib/libtritonserver.so /usr/lib/; \
    elif [ -f /opt/tritonserver/lib/libtritonserver.so ]; then \
        echo "Copying Triton library to system path" && \
        cp /opt/tritonserver/lib/libtritonserver.so /usr/lib/; \
    fi && \
    ldconfig

# Copy SDK include files to system location (from edgemlsdk/include/Panorama)
RUN mkdir -p /usr/local/include && \
    if [ -d /tmp/edgemlsdk/include/Panorama ]; then \
        cp -r /tmp/edgemlsdk/include/Panorama /usr/local/include/; \
    elif [ -d /tmp/edgemlsdk/src/include/Panorama ]; then \
        cp -r /tmp/edgemlsdk/src/include/Panorama /usr/local/include/; \
    fi && \
    ls -la /usr/local/include/Panorama/ 2>/dev/null || echo "No Panorama headers found"

# Create build directory
WORKDIR /app/build

# Copy source files
COPY CMakeLists.txt /app/
COPY include/ /app/include/
COPY src/ /app/src/

# Build the application
WORKDIR /app/build
RUN cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DEDGEML_SDK_PATH=/tmp/edgemlsdk \
    && cmake --build . -j$(nproc) \
    && cmake --install .

# ============================================
# Runtime stage
# ============================================
FROM public.ecr.aws/ubuntu/ubuntu:${OS} AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies including Python 3.9 for Triton Python backend
RUN apt-get update && apt-get install -y --no-install-recommends \
    libsqlite3-0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav \
    libgstreamer1.0-0 \
    libgstreamer-plugins-base1.0-0 \
    libssl3 \
    libglib2.0-0 \
    libxml2 \
    libusb-1.0-0 \
    libb64-0d \
    ffmpeg \
    curl \
    software-properties-common \
    libarchive13 \
    libexif12 \
    libcurl4 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install Python 3.9 (required for Triton Python backend)
RUN apt-get update && \
    apt-get install -y gnupg && \
    add-apt-repository ppa:deadsnakes/ppa -y && \
    apt-get update && \
    apt-get install -y python3.9 python3.9-dev python3.9-distutils python3-pip && \
    update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.9 1 && \
    python3.9 -m pip install --upgrade pip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Python packages required by Triton Python backend and model conversion
# Matches model_conversion_requirements.txt and Flask backend requirements
RUN python3.9 -m pip install --no-cache-dir \
    setuptools \
    wheel \
    meson \
    boto3 \
    dill \
    dlr==1.10.0 \
    grpcio==1.56.2 \
    grpcio-tools==1.51.1 \
    numpy==1.24.3 \
    opencv-python-headless \
    pillow==10.3.0 \
    protobuf==4.25.8 \
    requests==2.32.3 \
    scikit-learn==1.0.2 \
    urllib3==2.2.3

# Copy built artifacts from builder
COPY --from=builder /usr/local/bin/dda_backend_cpp /usr/local/bin/
COPY --from=builder /usr/local/lib/ /usr/local/lib/

# Copy Panorama SDK libraries from builder
COPY --from=builder /usr/lib/libpanorama.* /usr/lib/
COPY --from=builder /usr/lib/libtritonserver.so /usr/lib/

# Copy Triton installation files from builder
COPY --from=builder /opt/tritonserver /opt/tritonserver

COPY --from=builder /usr/local/share/gir-1.0/ /usr/local/share/gir-1.0/

# Copy Python backend modules for Triton Python backend model processing
# Note: These are copied from Flask backend context during Greengrass build
# For standalone builds, ensure these are available in the build context
WORKDIR /app
COPY dda_triton ./dda_triton
COPY lyra_anomalies_mask_utils ./lyra_anomalies_mask_utils
COPY lyra_science_processing_utils ./lyra_science_processing_utils
COPY dlr_disable_phone_home.py ./

# Install unzip for model extraction
RUN apt-get update && apt-get install -y unzip && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Disable DLR phone home
RUN python3.9 dlr_disable_phone_home.py || echo "DLR phone home disable skipped"

# Add Python modules to PYTHONPATH
ENV PYTHONPATH=/app:$PYTHONPATH

# Update library cache
RUN ldconfig

# Create work directories
RUN mkdir -p /aws_dda/triton/model_repository \
    && mkdir -p /aws_dda/image-capture/preview \
    && mkdir -p /aws_dda/inference-results \
    && mkdir -p /var/log/dda

# Create entrypoint script to copy Python scripts to /aws_dda (for Greengrass lifecycle)
RUN echo '#!/bin/bash\n\
# Copy Python scripts to /aws_dda for Greengrass model component lifecycle\n\
cp -n /app/dda_triton/model_convertor.py /aws_dda/ 2>/dev/null || true\n\
cp -n /app/dda_triton/convert_model_cleanup.py /aws_dda/ 2>/dev/null || true\n\
cp -rn /app/dda_triton /aws_dda/ 2>/dev/null || true\n\
cp -rn /app/lyra_anomalies_mask_utils /aws_dda/ 2>/dev/null || true\n\
cp -rn /app/lyra_science_processing_utils /aws_dda/ 2>/dev/null || true\n\
exec "$@"' > /entrypoint.sh && chmod +x /entrypoint.sh

# Set environment variables
ENV COMPONENT_WORK_PATH=/aws_dda
ENV TRITON_MODEL_DIR=/aws_dda/triton/model_repository
ENV TRITON_INSTALLATION_DIR=/aws_dda/triton
ENV DDA_SERVER_PORT=5000
ENV DDA_LOG_LEVEL=info
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Expose ports
EXPOSE 5000
EXPOSE 5443

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:5000/system/health || exit 1

USER root
ENTRYPOINT ["/entrypoint.sh"]
CMD ["/usr/local/bin/dda_backend_cpp"]
