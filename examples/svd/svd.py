#!/usr/bin/python

# Copyright (c) 2014-2015, Intel Corporation All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function

import sys
import numpy as np
from PIL import Image

import pymic as mic

# get handle to offload device, get stream
device = mic.devices[0]
library = device.load_library("libsvd.so")
stream = device.get_default_stream()


def read_image(file):
    """Read a JPEG file and convert it to gray scale"""

    # open the image and convert it to gray scale
    print("Reading file '{0}'".format(file))
    img = Image.open(file)
    img = img.convert("LA")
    print("Image size: {0}x{1}".format(img.size[1], img.size[0]))
    return img


def write_image(image, file):
    """Write an image as JPEG to disk"""

    # convert back to RGB and save
    print("Saving file '{0}'".format(file))
    image = image.convert("RGB")
    image.save(file)


def compute_svd(image):
    # convert the gray-scale image into a numpy.matrix
    print("Creating matrix")
    mtx = np.asarray(image.getdata(band=0), float)
    mtx.shape = (image.size[1], image.size[0])
    mtx = np.matrix(mtx)

    # run the SVD and return the matrixes: U, sigma, V
    print("Computing SVD")
    U, sigma, V = np.linalg.svd(mtx)
    return U, sigma, V


def reconstruct_image(U, sigma, V):
    """Reconstruct the image from the SVD data"""

    print("Reconstructing image on the host")

    # reconstruction of the image data
    reconstructed = U * sigma * V

    # reconstruct an Image object
    image = Image.fromarray(reconstructed)
    return image


def reconstruct_image_dgemm(U, sigma, V):
    """Reconstruct the image from the SVD data (using plain dgemm on MIC)"""

    print("Reconstructing image on the coprocessor")

    # create offload buffers
    offl_tmp = stream.empty((U.shape[0], U.shape[1]),
                            dtype=float, update_host=False)
    offl_res = stream.empty((U.shape[0], V.shape[1]),
                            dtype=float, update_host=False)
    offl_U = stream.bind(U)
    offl_sigma = stream.bind(sigma)
    offl_V = stream.bind(V)

    alpha = 1.0
    beta = 0.0

    # tmp = U * sigma
    transA = int(not U.flags.c_contiguous)
    transB = int(not sigma.flags.c_contiguous)
    m, k, n = U.shape[0], U.shape[1], sigma.shape[1]
    stream.invoke(library.dgemm_kernel,
                  offl_U, transA, offl_sigma, transB, offl_tmp,
                  m, n, k, alpha, beta)

    # res = tmp * V
    m, k, n = offl_tmp.shape[0], offl_tmp.shape[1], V.shape[1]
    transA = int(not offl_tmp.array.flags.c_contiguous)
    transB = int(not V.flags.c_contiguous)
    stream.invoke(library.dgemm_kernel,
                  offl_tmp, transA, offl_V, transB, offl_res,
                  m, n, k, alpha, beta)
    res = offl_res.update_host().array
    stream.sync()

    image = Image.fromarray(res)
    return image


# low man's command-line parsing
if len(sys.argv) != 4:
    print("Usage: {0} input.jpg compression output".format(sys.argv[0]))
    quit()
filename = sys.argv[1]
compression = int(sys.argv[2])
output = sys.argv[3]

# read image and create SVD
image = read_image(filename)
U, S, V = compute_svd(image)

# invoke empty kernel to have a hot team on the target
stream.invoke(library.empty)
stream.sync()

# reconstruct a compressed image (host)
Uc = np.matrix(U[:, :compression])
Sc = np.diag(S[:compression])
Vc = np.matrix(V[:compression, :])
recon = reconstruct_image(Uc, Sc, Vc)
write_image(recon, "host_{0}_{1:04}.jpg".format(output, compression))

# reconstruct a compressed image (offloaded)
Uc = np.matrix(U[:, :compression])
Sc = np.diag(S[:compression])
Vc = np.matrix(V[:compression, :])
recon = reconstruct_image_dgemm(Uc, Sc, Vc)
write_image(recon, "dgemm_{0}_{1:04}.jpg".format(output, compression))
