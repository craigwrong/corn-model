import Foundation
import CryptoKit

public func hash256(_ data: Data) -> Data {
    let digest = SHA256.hash(data: sha256(data))
    return Data(digest)
}
