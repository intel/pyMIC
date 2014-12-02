#!/usr/bin/python

# Copyright (c) 2014, Intel Corporation All rights reserved. 
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

import sys
import numpy as np
from PIL import Image

import pyMIC as mic

# get handle to offload device
device = mic.devices[0]
device.load_library("libsvd.so")

def read_image(file):
    """Read a JPEG file and convert it to gray scale"""
    
    # open the image and convert it to gray scale
    print "Reading file '{0}'".format(file)
    img = Image.open(file)
    img = img.convert("LA")
    print "Image size: {0}x{1}".format(img.size[1], img.size[0])
    return img

def write_image(image, file):
    """Write an image as JPEG to disk"""
    
    # convert back to RGB and save
    # print "Saving file '{0}'".format(file)
    image = image.convert("RGB")
    image.save(file)
    
def compute_svd(image):    
    # convert the gray-scale image into a numpy.matrix
    print "Creating matrix"
    mtx = np.asarray(image.getdata(band=0), float)
    mtx.shape = (image.size[1], image.size[0])
    mtx = np.matrix(mtx)
    
    # run the SVD and return the matrixes: U, sigma, V
    print "Computing SVD"
    return np.linalg.svd(mtx)

def reconstruct_image(U, sigma, V):
    """Reconstruct the image from the SVD data"""
    
    # reconstruction of the image data
    reconstructed = U * sigma * V
    
    # reconstruct an Image object
    image = Image.fromarray(reconstructed)
    return image
    
def reconstruct_image_dgemm(U, sigma, V):
    """Reconstruct the image from the SVD data (using plain dgemm on MIC)"""
    
    # create offload buffers
    offl_tmp   = device.empty((U.shape[0], U.shape[1]), dtype=float, update_host=False)
    offl_res   = device.empty((U.shape[0], V.shape[1]), dtype=float, update_host=False)
    offl_U     = device.bind(U)
    offl_sigma = device.bind(sigma)
    offl_V     = device.bind(V)
        
    alpha = 1.0
    beta  = 0.0
    
    # tmp = U * sigma 
    m, k, n = U.shape[0], U.shape[1], sigma.shape[1]
    device.invoke_kernel("dgemm_kernel", 
                         offl_U, offl_sigma, offl_tmp, 
                         m, n, k, alpha, beta)

    # res = tmp * V 
    m, k, n = offl_tmp.shape[0], offl_tmp.shape[1], V.shape[1]
    device.invoke_kernel("dgemm_kernel", 
                         offl_tmp, offl_V, offl_res, 
                         m, n, k, alpha, beta)
    
    image = Image.fromarray(offl_res.update_host().array)
    return image
    
def reconstruct_image_svd(U, sigma, V):
    """Reconstruct the image from the SVD data (using a special SVD kernel)"""
    
    # create offload buffers
    offl_res   = device.empty((U.shape[0], V.shape[1]), dtype=float, update_host=False)
    offl_U     = device.bind(U)
    offl_sigma = device.bind(sigma)
    offl_V     = device.bind(V)
    
    # dimensions of the matrixes
    x, y, z = U.shape[0], V.shape[1], sigma.shape[0]
    
    # perform the complete offload
    device.invoke_kernel("svd_reconstruct", 
                         offl_U, offl_sigma, offl_V, offl_res,
                         x, y, z)
    
    image = Image.fromarray(offl_res.update_host().array)
    return image
    
# low man's command-line parsing
filename = sys.argv[1]
output = sys.argv[2]

# read image and create SVD
image = read_image(filename)
U,S,V = compute_svd(image)

# invoke empty kernel to have a hot team on the target
device.invoke_kernel("empty")

# reconstruct some images (host)
for i in xrange(1, 20, 1):
    Uc = np.matrix(U[:, :i])
    Sc = np.diag(S[:i])
    Vc = np.matrix(V[:i,:]) 
    recon = reconstruct_image(Uc,Sc,Vc)
    write_image(recon, "host_{0}_{1:04}.jpg".format(output, i))

# simple dgemm offload
for i in xrange(1, 20, 1):
    Uc = np.matrix(U[:, :i])
    Sc = np.diag(S[:i])
    Vc = np.matrix(V[:i,:])
    recon = reconstruct_image_dgemm(Uc,Sc,Vc)
    write_image(recon, "dgemm_{0}_{1:04}.jpg".format(output, i))

# special SVD reconstruction kernel
for i in xrange(1, 20, 1):
    Uc = np.matrix(U[:, :i])
    Sc = np.diag(S[:i])
    Vc = np.matrix(V[:i,:])
    recon = reconstruct_image_svd(Uc,Sc,Vc)
    write_image(recon, "svd_{0}_{1:04}.jpg".format(output, i))

