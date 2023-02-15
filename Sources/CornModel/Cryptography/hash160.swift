import Foundation

public func hash160(_ data: Data) -> Data {
    RIPEMD160.hash(singleHash(data))
}
