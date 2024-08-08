/*
 * Copyright (C) 2024, Gerald Kimmersdorfer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

vec4 applyPermutation(vec4 src, uint permutationKey) {
    switch (permutationKey) {
        case 0: return vec4(src[0], src[1], src[2], src[3]);
        case 1: return vec4(src[0], src[1], src[3], src[2]);
        case 2: return vec4(src[0], src[2], src[1], src[3]);
        case 3: return vec4(src[0], src[2], src[3], src[1]);
        case 4: return vec4(src[0], src[3], src[2], src[1]);
        case 5: return vec4(src[0], src[3], src[1], src[2]);
        case 6: return vec4(src[1], src[0], src[2], src[3]);
        case 7: return vec4(src[1], src[0], src[3], src[2]);
        case 8: return vec4(src[1], src[2], src[0], src[3]);
        case 9: return vec4(src[1], src[2], src[3], src[0]);
        case 10: return vec4(src[1], src[3], src[2], src[0]);
        case 11: return vec4(src[1], src[3], src[0], src[2]);
        case 12: return vec4(src[2], src[1], src[0], src[3]);
        case 13: return vec4(src[2], src[1], src[3], src[0]);
        case 14: return vec4(src[2], src[0], src[1], src[3]);
        case 15: return vec4(src[2], src[0], src[3], src[1]);
        case 16: return vec4(src[2], src[3], src[0], src[1]);
        case 17: return vec4(src[2], src[3], src[1], src[0]);
        case 18: return vec4(src[3], src[1], src[2], src[0]);
        case 19: return vec4(src[3], src[1], src[0], src[2]);
        case 20: return vec4(src[3], src[2], src[1], src[0]);
        case 21: return vec4(src[3], src[2], src[0], src[1]);
        case 22: return vec4(src[3], src[0], src[2], src[1]);
        case 23: return vec4(src[3], src[0], src[1], src[2]);
    }
    return src; // Shouldnt happen unless permutationKey is invalid
}

uvec4 applyPermutation(uvec4 src, uint permutationKey) {
    switch (permutationKey) {
        case 0: return uvec4(src[0], src[1], src[2], src[3]);
        case 1: return uvec4(src[0], src[1], src[3], src[2]);
        case 2: return uvec4(src[0], src[2], src[1], src[3]);
        case 3: return uvec4(src[0], src[2], src[3], src[1]);
        case 4: return uvec4(src[0], src[3], src[2], src[1]);
        case 5: return uvec4(src[0], src[3], src[1], src[2]);
        case 6: return uvec4(src[1], src[0], src[2], src[3]);
        case 7: return uvec4(src[1], src[0], src[3], src[2]);
        case 8: return uvec4(src[1], src[2], src[0], src[3]);
        case 9: return uvec4(src[1], src[2], src[3], src[0]);
        case 10: return uvec4(src[1], src[3], src[2], src[0]);
        case 11: return uvec4(src[1], src[3], src[0], src[2]);
        case 12: return uvec4(src[2], src[1], src[0], src[3]);
        case 13: return uvec4(src[2], src[1], src[3], src[0]);
        case 14: return uvec4(src[2], src[0], src[1], src[3]);
        case 15: return uvec4(src[2], src[0], src[3], src[1]);
        case 16: return uvec4(src[2], src[3], src[0], src[1]);
        case 17: return uvec4(src[2], src[3], src[1], src[0]);
        case 18: return uvec4(src[3], src[1], src[2], src[0]);
        case 19: return uvec4(src[3], src[1], src[0], src[2]);
        case 20: return uvec4(src[3], src[2], src[1], src[0]);
        case 21: return uvec4(src[3], src[2], src[0], src[1]);
        case 22: return uvec4(src[3], src[0], src[2], src[1]);
        case 23: return uvec4(src[3], src[0], src[1], src[2]);
    }
    return src; // Shouldnt happen unless permutationKey is invalid
}

vec4 applyPermutationInverse(vec4 src, uint permutationKey) {
    switch (permutationKey) {
        case 0: return vec4(src[0], src[1], src[2], src[3]);
        case 1: return vec4(src[0], src[1], src[3], src[2]);
        case 2: return vec4(src[0], src[2], src[1], src[3]);
        case 3: return vec4(src[0], src[3], src[1], src[2]);
        case 4: return vec4(src[0], src[3], src[2], src[1]);
        case 5: return vec4(src[0], src[2], src[3], src[1]);
        case 6: return vec4(src[1], src[0], src[2], src[3]);
        case 7: return vec4(src[1], src[0], src[3], src[2]);
        case 8: return vec4(src[2], src[0], src[1], src[3]);
        case 9: return vec4(src[3], src[0], src[1], src[2]);
        case 10: return vec4(src[3], src[0], src[2], src[1]);
        case 11: return vec4(src[2], src[0], src[3], src[1]);
        case 12: return vec4(src[2], src[1], src[0], src[3]);
        case 13: return vec4(src[3], src[1], src[0], src[2]);
        case 14: return vec4(src[1], src[2], src[0], src[3]);
        case 15: return vec4(src[1], src[3], src[0], src[2]);
        case 16: return vec4(src[2], src[3], src[0], src[1]);
        case 17: return vec4(src[3], src[2], src[0], src[1]);
        case 18: return vec4(src[3], src[1], src[2], src[0]);
        case 19: return vec4(src[2], src[1], src[3], src[0]);
        case 20: return vec4(src[3], src[2], src[1], src[0]);
        case 21: return vec4(src[2], src[3], src[1], src[0]);
        case 22: return vec4(src[1], src[3], src[2], src[0]);
        case 23: return vec4(src[1], src[2], src[3], src[0]);
    }
    return src; // Shouldnt happen unless permutationKey is invalid
}

uvec4 applyPermutationInverse(uvec4 src, uint permutationKey) {
    switch (permutationKey) {
        case 0: return uvec4(src[0], src[1], src[2], src[3]);
        case 1: return uvec4(src[0], src[1], src[3], src[2]);
        case 2: return uvec4(src[0], src[2], src[1], src[3]);
        case 3: return uvec4(src[0], src[3], src[1], src[2]);
        case 4: return uvec4(src[0], src[3], src[2], src[1]);
        case 5: return uvec4(src[0], src[2], src[3], src[1]);
        case 6: return uvec4(src[1], src[0], src[2], src[3]);
        case 7: return uvec4(src[1], src[0], src[3], src[2]);
        case 8: return uvec4(src[2], src[0], src[1], src[3]);
        case 9: return uvec4(src[3], src[0], src[1], src[2]);
        case 10: return uvec4(src[3], src[0], src[2], src[1]);
        case 11: return uvec4(src[2], src[0], src[3], src[1]);
        case 12: return uvec4(src[2], src[1], src[0], src[3]);
        case 13: return uvec4(src[3], src[1], src[0], src[2]);
        case 14: return uvec4(src[1], src[2], src[0], src[3]);
        case 15: return uvec4(src[1], src[3], src[0], src[2]);
        case 16: return uvec4(src[2], src[3], src[0], src[1]);
        case 17: return uvec4(src[3], src[2], src[0], src[1]);
        case 18: return uvec4(src[3], src[1], src[2], src[0]);
        case 19: return uvec4(src[2], src[1], src[3], src[0]);
        case 20: return uvec4(src[3], src[2], src[1], src[0]);
        case 21: return uvec4(src[2], src[3], src[1], src[0]);
        case 22: return uvec4(src[1], src[3], src[2], src[0]);
        case 23: return uvec4(src[1], src[2], src[3], src[0]);
    }
    return src; // Shouldnt happen unless permutationKey is invalid
}